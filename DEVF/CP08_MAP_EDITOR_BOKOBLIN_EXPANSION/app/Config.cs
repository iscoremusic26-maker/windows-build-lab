using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
namespace DevfMap;
public sealed record Encounter(string id,bool enabled,string enemy,string stage,int room,int layer,double x,double y,double z,int yaw_s16);
public sealed record Document(string document_type,int schema_version,string geometry_sha256,List<Encounter> encounters) {
 public static Document Empty()=>new("devf_map_encounters",1,Geometry.Hash,new());
}
public static class Config {
 public static readonly JsonSerializerOptions Options=new(){WriteIndented=true};
 public static byte[] Read(string path){
  using var stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read);
  if(stream.Length<1||stream.Length>16384)throw new InvalidDataException("Archivo JSON vacío o mayor que 16 KiB");
  var bytes=new byte[(int)stream.Length];stream.ReadExactly(bytes);
  if(stream.ReadByte()!=-1)throw new IOException("Archivo cambió durante la lectura");
  return bytes;
 }
 public static string Hash(byte[] b)=>Convert.ToHexString(SHA256.HashData(b)).ToLowerInvariant();
 public static byte[] Encode(Document d){Validate(d);var bytes=JsonSerializer.SerializeToUtf8Bytes(d,Options);_ = Decode(bytes);return bytes;}
 static void Keys(JsonElement e,params string[] keys){if(e.ValueKind!=JsonValueKind.Object)throw new InvalidDataException("Objeto esperado");var names=e.EnumerateObject().Select(p=>p.Name).ToArray();if(names.Length!=keys.Length||names.Distinct().Count()!=names.Length||keys.Any(k=>!names.Contains(k)))throw new InvalidDataException("Claves desconocidas/duplicadas");}
 public static Document Decode(byte[] b){
  if(b.Length==0||b.Length>16384||b.Any(v=>v>127)||b.Contains((byte)'\\'))throw new InvalidDataException("JSON ASCII sin escapes, máximo 16 KiB");
  using var json=JsonDocument.Parse(b,new JsonDocumentOptions{MaxDepth=8});var root=json.RootElement;Keys(root,"document_type","schema_version","geometry_sha256","encounters");
  foreach(var e in root.GetProperty("encounters").EnumerateArray())Keys(e,"id","enabled","enemy","stage","room","layer","x","y","z","yaw_s16");
  var d=JsonSerializer.Deserialize<Document>(b)??throw new InvalidDataException("JSON vacío");Validate(d);return d;
 }
 public static void Validate(Document d){
  if(d.document_type!="devf_map_encounters"||d.schema_version!=1||d.geometry_sha256!=Geometry.Hash||d.encounters==null||d.encounters.Count>8)throw new InvalidDataException("Formato/versión/capacidad no admitidos");
  for(int i=0;i<d.encounters.Count;i++){var a=d.encounters[i];
   if(a==null||string.IsNullOrEmpty(a.id)||a.id.Length>31||a.id.Any(c=>!(c is >= 'a' and <= 'z' or >= '0' and <= '9' or '_' or '-'))||a.enemy!="E_OC2"||a.stage!="F_SP121"||a.room!=3||a.layer!=6||a.yaw_s16< -32768||a.yaw_s16>32767||!double.IsFinite(a.x)||!double.IsFinite(a.y)||!double.IsFinite(a.z)||a.x<Geometry.MinX||a.x>Geometry.MaxX||a.y< -7500||a.y> -5000||a.z<Geometry.MinZ||a.z>Geometry.MaxZ)throw new InvalidDataException("Encuentro incompatible: ID, escena, coordenadas u orientación");
   for(int j=0;j<i;j++){var b=d.encounters[j];double dx=(float)a.x-(float)b.x,dz=(float)a.z-(float)b.z;
    if(a.id==b.id||((float)a.x==(float)b.x&&(float)a.y==(float)b.y&&(float)a.z==(float)b.z))throw new InvalidDataException("ID o posición duplicados");
    if(a.enabled&&b.enabled&&dx*dx+dz*dz<90000)throw new InvalidDataException("Separación mínima entre activos: 300 unidades");
   }
  }
 }
 public static void GeometryCheck(Document d,Geometry g){foreach(var e in d.encounters){var h=g.Place((float)e.x,(float)e.z);if(Math.Abs(h-(float)e.y)>.01)throw new InvalidDataException("Altura no coincide con la colisión importada: "+e.id);}}
 public static string Save(string path,Document d,string? expectedHash,Action<string>? inject=null){
  var bytes=Encode(d);var dir=Path.GetDirectoryName(Path.GetFullPath(path))!;Directory.CreateDirectory(dir);
  byte[]? previous=File.Exists(path)?Read(path):null;
  if((previous==null?null:Hash(previous))!=expectedHash)throw new IOException("Archivo cambiado externamente. Abra de nuevo; no se sobrescribió.");
  string backup="",temp=Path.Combine(dir,".devf-"+Guid.NewGuid().ToString("N")+".tmp");bool replaced=false;
  try{
   if(previous!=null){Decode(previous);backup=path+".backup-"+DateTime.UtcNow.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N")+".json";
    inject?.Invoke("backup");using(var s=new FileStream(backup,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(previous);s.Flush(true);}
    if(!Read(backup).SequenceEqual(previous))throw new IOException("Respaldo no verificado");
   }
   inject?.Invoke("write");using(var s=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(bytes);inject?.Invoke("flush");s.Flush(true);}inject?.Invoke("close");
   if(!Read(temp).SequenceEqual(bytes))throw new IOException("Temporal no verificado");Decode(Read(temp));
   // Check again before replacement. This is not a cross-process transaction.
   if((File.Exists(path)?Hash(Read(path)):null)!=expectedHash)throw new IOException("Conflicto externo antes de reemplazar");
   File.Move(temp,path,true);replaced=true;inject?.Invoke("verify");var read=Read(path);Decode(read);if(!read.SequenceEqual(bytes))throw new IOException("Relectura distinta");
   return backup;
  }catch{
   if(replaced){if(previous!=null)File.Copy(backup,path,true);else File.Delete(path);}
   throw;
  }finally{if(File.Exists(temp))File.Delete(temp);}
 }
}
