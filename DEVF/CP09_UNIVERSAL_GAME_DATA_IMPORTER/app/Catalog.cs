using System.Text.Json;
using System.Text.RegularExpressions;
namespace DevfMap;
public sealed record ResourceInfo {
 public string Path{get;init;}="";public string Stage{get;init;}="";public int? Room{get;init;} public string Variant{get;init;}="";
 public string IdentityOrigin{get;init;}="FST path (room number inferred from Rxx suffix; PLC rooms independent)";
 public long Offset{get;init;}public int Bytes{get;init;}public string Sha256{get;set;}="";public string Status{get;set;}="NOT_PROCESSED";
 public string[] LayerChunks{get;set;}=Array.Empty<string>();public string LayerMeaning{get;init;}="DZX declared chunks only; active layer UNKNOWN";
 public int[] PlcRooms{get;set;}=Array.Empty<int>();public double[] Bounds{get;set;}=Array.Empty<double>();public int Triangles{get;set;}
 public string KclPath{get;set;}="";public string PlcPath{get;set;}="";public string KclHash{get;set;}="";public string PlcHash{get;set;}="";
 public bool Cp08Compatible=>Stage=="F_SP121"&&Room==3&&Sha256==Geometry.Hash&&Status=="GEOMETRY_AVAILABLE";
}
public sealed record GameCatalog {
 public int SchemaVersion{get;init;}=1;public string Importer{get;init;}="0.9.0";public string ImageSha256{get;init;}="";
 public string Format{get;init;}="";public string DiscId{get;init;}="";public int Revision{get;init;}public string FstSha256{get;init;}="";
 public List<ResourceInfo> Resources{get;init;}=new();
}
public sealed class CatalogService {
 static readonly Regex Pattern=new(@"(?:^|/)Stage/([A-Za-z0-9_]{1,32})/(R([0-9]{2})_([0-9]{2})|STG_([0-9]{2}))\.arc$",RegexOptions.IgnoreCase|RegexOptions.CultureInvariant);
 public static string LocalRoot=>Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DEVF","MapEditor","CP09");
 public static JsonSerializerOptions Options{get;}=new(){WriteIndented=true,MaxDepth=32};
 public static List<ResourceInfo> Discover(Disc disc){
  var result=new List<ResourceInfo>();foreach(var e in disc.Files){var m=Pattern.Match(e.Path);if(!m.Success)continue;
   Disc.Require(result.Count<10000,"RESOURCE_COUNT_LIMIT");result.Add(new(){Path=e.Path,Stage=m.Groups[1].Value,Room=m.Groups[3].Success?int.Parse(m.Groups[3].Value):null,Variant=m.Groups[4].Success?m.Groups[4].Value:m.Groups[5].Value,Offset=e.Offset,Bytes=e.Length});
  }return result;
 }
 static void ValidateCache(GameCatalog c,string hash,Disc disc,List<ResourceInfo> expected){
  Disc.Require(c.SchemaVersion==1&&c.Importer=="0.9.0"&&c.ImageSha256==hash&&c.DiscId==disc.Id&&c.Revision==disc.Revision&&c.FstSha256==disc.FstHash&&c.Resources.Count==expected.Count,"CACHE_IDENTITY");
  for(int i=0;i<expected.Count;i++){var a=c.Resources[i];var b=expected[i];Disc.Require(a.Path==b.Path&&a.Stage==b.Stage&&a.Room==b.Room&&a.Variant==b.Variant&&a.Offset==b.Offset&&a.Bytes==b.Bytes&&a.Sha256.Length is 0 or 64&&a.Bounds.Length is 0 or 6&&a.Bounds.All(double.IsFinite)&&a.Triangles is >=0 and <=200000,"CACHE_RESOURCE");}
 }
 public static string CachePath(string root,string hash){Disc.Require(hash.Length==64&&hash.All(Uri.IsHexDigit),"CACHE_KEY");return Path.Combine(root,"catalog-v1-"+hash+".json");}
 public static GameCatalog Import(string image,string root,CancellationToken cancel,IProgress<string>? progress=null){
  using var disc=new Disc(image);string hash=Disc.ImageHash(image,cancel,progress);var expected=Discover(disc);string cache=CachePath(root,hash);
  if(File.Exists(cache)){try{Disc.Require(new FileInfo(cache).Length<=16*1024*1024,"CACHE_SIZE");var c=JsonSerializer.Deserialize<GameCatalog>(File.ReadAllBytes(cache),Options)??throw new InvalidDataException("CACHE_NULL");ValidateCache(c,hash,disc,expected);progress?.Report("Catálogo verificado en caché");return c;}catch(Exception ex)when(ex is IOException or JsonException or InvalidDataException or ArgumentException){progress?.Report("Caché inválida; se reconstruye.");}}
  var catalog=new GameCatalog{ImageSha256=hash,Format=disc.Format,DiscId=disc.Id,Revision=disc.Revision,FstSha256=disc.FstHash,Resources=expected};
  for(int i=0;i<expected.Count;i++){cancel.ThrowIfCancellationRequested();var r=expected[i];progress?.Report($"Recurso {i+1}/{expected.Count} · {r.Stage} / sala {r.Room?.ToString()??"stage"}");
   try{
    Disc.Require(r.Bytes>=4&&r.Bytes<=Archive.Max,"RESOURCE_SIZE");var raw=disc.Read(r.Offset,r.Bytes);r.Sha256=Disc.Hash(raw);var members=Archive.Read(raw,cancel);r.LayerChunks=Archive.LayerChunks(members);
    var mesh=GeometryReader.Read(members,cancel);r.Bounds=new[]{mesh.MinX,mesh.MaxX,mesh.MinY,mesh.MaxY,mesh.MinZ,mesh.MaxZ};r.PlcRooms=mesh.PlcRooms;r.Triangles=mesh.Triangles.Count;r.KclPath=mesh.KclPath;r.PlcPath=mesh.PlcPath;r.KclHash=mesh.KclHash;r.PlcHash=mesh.PlcHash;r.Status="GEOMETRY_AVAILABLE";
   }catch(OperationCanceledException){throw;}catch(Exception ex)when(ex is InvalidDataException or IOException or ArgumentException or OverflowException or InvalidOperationException){
    r.Status=ex is InvalidDataException?ex.Message:"RESOURCE_MALFORMED";r.Bounds=Array.Empty<double>();r.Triangles=0;
   }
  }
  cancel.ThrowIfCancellationRequested();var encoded=JsonSerializer.SerializeToUtf8Bytes(catalog,Options);Disc.Require(encoded.Length<=16*1024*1024,"CATALOG_SIZE");
  Directory.CreateDirectory(root);string temp=cache+".tmp-"+Guid.NewGuid().ToString("N");
  try{using(var s=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(encoded);s.Flush(true);}
   var read=File.ReadAllBytes(temp);Disc.Require(read.SequenceEqual(encoded),"CACHE_WRITE_VERIFY");ValidateCache(JsonSerializer.Deserialize<GameCatalog>(read,Options)!,hash,disc,expected);cancel.ThrowIfCancellationRequested();File.Move(temp,cache,true);
  }finally{if(File.Exists(temp))File.Delete(temp);}return catalog;
 }
 public static (Mesh Mesh,byte[] Raw) Load(string image,GameCatalog catalog,ResourceInfo resource,CancellationToken cancel){
  using var disc=new Disc(image);Disc.Require(disc.Id==catalog.DiscId&&disc.FstHash==catalog.FstSha256,"IMAGE_CHANGED");
  var entry=disc.Files.SingleOrDefault(x=>x.Path==resource.Path);Disc.Require(entry!=null&&entry.Offset==resource.Offset&&entry.Length==resource.Bytes,"RESOURCE_CHANGED");
  cancel.ThrowIfCancellationRequested();var raw=disc.Read(resource.Offset,resource.Bytes);Disc.Require(Disc.Hash(raw)==resource.Sha256,"RESOURCE_HASH_CHANGED");return (GeometryReader.Read(Archive.Read(raw,cancel),cancel),raw);
 }
 public static void Remember(string image){Directory.CreateDirectory(LocalRoot);string file=Path.Combine(LocalRoot,"last-image.txt");File.WriteAllText(file,image);}
 public static string? LastImage(){string f=Path.Combine(LocalRoot,"last-image.txt");return File.Exists(f)&&new FileInfo(f).Length<4096?File.ReadAllText(f):null;}
}
