using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
namespace DevfMap;
internal static class ImporterTests {
 sealed class ProgressAction(Action<string> action):IProgress<string>{public void Report(string s)=>action(s);}

 static void U(byte[] b,int p,uint v)=>BinaryPrimitives.WriteUInt32BigEndian(b.AsSpan(p,4),v);
 static void S(byte[] b,int p,ushort v)=>BinaryPrimitives.WriteUInt16BigEndian(b.AsSpan(p,2),v);
 static void F(byte[] b,int p,float v)=>U(b,p,unchecked((uint)BitConverter.SingleToInt32Bits(v)));
 static byte[] Room(int room,float x=0){
  var k=new byte[152];U(k,0,56);U(k,4,68);U(k,8,116);U(k,12,148);F(k,56,x);
  float[][] norms={new[]{0f,1f,0f},new[]{-1f,0f,0f},new[]{0f,0f,-1f},new[]{.70710677f,0f,.70710677f}};
  for(int i=0;i<4;i++)for(int j=0;j<3;j++)F(k,68+i*12+j*4,norms[i][j]);
  F(k,132,70.71068f);S(k,138,0);S(k,140,1);S(k,142,2);S(k,144,3);
  var plc=new byte[28];"SPLC"u8.CopyTo(plc);S(plc,4,20);S(plc,6,1);U(plc,8,63);U(plc,24,(uint)room<<20);
  var dzr=new byte[16];U(dzr,0,1);"ACT6"u8.CopyTo(dzr.AsSpan(4));U(dzr,12,16);
  return Rarc(new(){["room.kcl"]=k,["room.plc"]=plc,["room.dzr"]=dzr});
 }
 static byte[] Rarc(Dictionary<string,byte[]> entries){
  var names=new List<byte>{0};var offsets=new List<int>();foreach(var n in entries.Keys){offsets.Add(names.Count);names.AddRange(Encoding.ASCII.GetBytes(n));names.Add(0);}
  int fileAt=80,stringAt=fileAt+entries.Count*20,dataAt=(stringAt+names.Count+31)&~31;var b=new byte[dataAt+entries.Sum(e=>e.Value.Length)];
  "RARC"u8.CopyTo(b);U(b,4,(uint)b.Length);U(b,8,32);U(b,12,(uint)(dataAt-32));U(b,32,1);U(b,36,32);U(b,40,(uint)entries.Count);U(b,44,48);U(b,48,(uint)names.Count);U(b,52,(uint)(stringAt-32));"ROOT"u8.CopyTo(b.AsSpan(64));S(b,74,(ushort)entries.Count);names.ToArray().CopyTo(b,stringAt);
  int i=0,offset=0;foreach(var e in entries){int at=fileAt+i*20;S(b,at,(ushort)i);U(b,at+4,0x11000000|(uint)offsets[i]);U(b,at+8,(uint)offset);U(b,at+12,(uint)e.Value.Length);e.Value.CopyTo(b,dataAt+offset);offset+=e.Value.Length;i++;}return b;
 }
 static byte[] AliasedRarc(){
  var b=new byte[196];"RARC"u8.CopyTo(b);U(b,4,196);U(b,8,32);U(b,12,160);U(b,32,2);U(b,36,32);U(b,40,3);U(b,44,64);U(b,48,8);U(b,52,124);
  S(b,74,2);S(b,90,1);U(b,92,2);Encoding.ASCII.GetBytes("\0a\0b\0f\0\0").CopyTo(b,156);
  U(b,100,0x02000001);U(b,104,1);U(b,120,0x02000003);U(b,124,1);U(b,140,0x11000005);U(b,148,4);
  return b;
 }
 static byte[] Iso(){
  var b=new byte[0x28000];Encoding.ASCII.GetBytes("GZ2E01").CopyTo(b,0);U(b,0x1c,0xc2339f3d);
  string[] names={"","res","Stage","T_TEST","R01_00.arc","R02_00.arc","R03_00.arc"};var strings=new List<byte>{0};int[] off=new int[7];
  for(int i=1;i<7;i++){off[i]=strings.Count;strings.AddRange(Encoding.ASCII.GetBytes(names[i]));strings.Add(0);}
  int fst=0x5000;U(b,0x424,(uint)fst);U(b,0x428,(uint)(84+strings.Count));U(b,fst,0x01000000);U(b,fst+8,7);
  for(int i=1;i<4;i++){U(b,fst+i*12,0x01000000|(uint)off[i]);U(b,fst+i*12+4,(uint)(i-1));U(b,fst+i*12+8,7);}
  byte[][] rooms={Room(1),Room(2,250),Encoding.ASCII.GetBytes("bad!")};
  for(int i=4;i<7;i++){int data=0x10000+(i-4)*0x8000;U(b,fst+i*12,(uint)off[i]);U(b,fst+i*12+4,(uint)data);U(b,fst+i*12+8,(uint)rooms[i-4].Length);rooms[i-4].CopyTo(b,data);}
  strings.ToArray().CopyTo(b,fst+84);return b;
 }
 static byte[] Ciso(byte[] iso){
  const int size=32768;var b=new List<byte>(new byte[32768]);b[0]=(byte)'C';b[1]=(byte)'I';b[2]=(byte)'S';b[3]=(byte)'O';var sz=BitConverter.GetBytes(size);for(int i=0;i<4;i++)b[4+i]=sz[i];
  for(int i=0;i<iso.Length/size;i++){var block=iso.AsSpan(i*size,size).ToArray();if(block.Any(v=>v!=0)){b[8+i]=1;b.AddRange(block);}}return b.ToArray();
 }
 public static void Run(string realArc,string output){
  var pass=new List<string>();void Ok(string n,bool v){if(!v)throw new Exception(n);pass.Add(n);}
  void Reject(string n,Action action){bool rejected=false;try{action();}catch(Exception e)when(e is InvalidDataException or IOException or OverflowException or ArgumentException){rejected=true;}Ok(n,rejected);}
  string work=Path.Combine(output,"synthetic-local");Directory.CreateDirectory(work);var iso=Iso();var ciso=Ciso(iso);
  string ip=Path.Combine(work,"legal-synthetic.iso"),cp=Path.Combine(work,"legal-synthetic.ciso");File.WriteAllBytes(ip,iso);File.WriteAllBytes(cp,ciso);
  using(var d=new Disc(cp)){Ok("CISO-sparse-real-reader",d.Id=="GZ2E01"&&d.Files.Count==3);Ok("CISO-cross-blocks-and-holes",d.Read(0,iso.Length).SequenceEqual(iso));Ok("FST-paths",d.Files[1].Path=="res/Stage/T_TEST/R02_00.arc");Reject("read-overflow",()=>d.Read(long.MaxValue,10));Reject("negative-read",()=>d.Read(-1,1));}
  using(var d=new Disc(ip)){Ok("ISO-logical-match",d.Read(0,iso.Length).SequenceEqual(iso));}
  void Bad(string name,byte[] bytes){string path=Path.Combine(work,"bad-image");File.WriteAllBytes(path,bytes);Reject(name,()=>{using var d=new Disc(path);});}
  Bad("truncated-disc",new byte[8]);Bad("truncated-CISO",ciso[..^1]);var bad=ciso.ToArray();bad[8]=2;Bad("CISO-map-invalid",bad);
  bad=ciso.ToArray();bad[4]=1;Bad("CISO-block-invalid",bad);bad=ciso.ToArray();"CISO"u8.CopyTo(bad);bad[7]=127;Bad("CISO-block-excess",bad);
  bad=iso.ToArray();bad[0]=(byte)'X';Bad("disc-ID-rejected",bad);bad=iso.ToArray();bad[7]=1;Bad("revision-not-certified",bad);
  bad=iso.ToArray();U(bad,0x424,uint.MaxValue);Bad("FST-offset-overflow",bad);bad=iso.ToArray();U(bad,0x5008,100001);Bad("FST-count-limit",bad);
  bad=iso.ToArray();U(bad,0x5000+12+4,3);Bad("FST-directory-parent",bad);
  bad=iso.ToArray();bad[0x5000+84+1]=(byte)'/';Bad("FST-path-traversal",bad);
  bad=iso.ToArray();U(bad,0x5000+5*12,Disc.BE(bad,0x5000+4*12));Bad("FST-duplicate-path",bad);
  bad=iso.ToArray();U(bad,0x5000+4*12+8,uint.MaxValue);Bad("FST-file-range",bad);
  var room=Room(1);var mesh=GeometryReader.Read(Archive.Read(room));Ok("generic-KCL-independent-room-bounds",mesh.Triangles.Count==1&&mesh.PlcRooms.SequenceEqual(new[]{1})&&Math.Abs(mesh.MaxX-100)<.001);
  Ok("generic-static-floor-height",Math.Abs(mesh.Ground(10,10,1))<.001);Reject("wrong-PLC-room-no-floor",()=>mesh.Ground(10,10,3));
  var t=mesh.Triangles[0];var upper=t with{P=t.P+new V(0,50,0),Q=t.Q+new V(0,50,0),R=t.R+new V(0,50,0)};
  Reject("overlapping-levels-ambiguous",()=> (mesh with{Triangles=new(){t,upper}}).Ground(10,10,1));
  Reject("CP08-hash-guard-no-export-other-room",()=>Geometry.LoadBytes(room));
  var yz=new byte[16];"Yaz0"u8.CopyTo(yz);U(yz,4,Archive.Max+1);Reject("Yaz0-expansion-cap",()=>Archive.Yaz(yz));U(yz,4,5);Reject("Yaz0-truncated",()=>Archive.Yaz(yz));
  var literal=new byte[22];"Yaz0"u8.CopyTo(literal);U(literal,4,5);literal[16]=0xf8;Encoding.ASCII.GetBytes("hello").CopyTo(literal,17);Ok("Yaz0-literal",Encoding.ASCII.GetString(Archive.Yaz(literal))=="hello");
  literal[16]=0;Reject("Yaz0-backref-underflow",()=>Archive.Yaz(literal));
  Reject("RARC-aliased-directory-graph",()=>Archive.Read(AliasedRarc()));Reject("RARC-truncated",()=>Archive.Read(room[..30]));bad=room.ToArray();U(bad,44,uint.MaxValue);Reject("RARC-table-overflow",()=>Archive.Read(bad));
  var files=Archive.Read(room).ToDictionary(e=>e.Key,e=>e.Value.ToArray());F(files["room.kcl"],56,float.NaN);Reject("KCL-NaN",()=>GeometryReader.Read(Archive.Read(Rarc(files))));
  files=Archive.Read(room).ToDictionary(e=>e.Key,e=>e.Value.ToArray());U(files["room.kcl"],12,uint.MaxValue);Reject("KCL-offset-overflow",()=>GeometryReader.Read(Archive.Read(Rarc(files))));
  files=Archive.Read(room).ToDictionary(e=>e.Key,e=>e.Value.ToArray());files["room.plc"]=files["room.plc"][..^1];Reject("PLC-truncated",()=>GeometryReader.Read(Archive.Read(Rarc(files))));
  string cache=Path.Combine(work,"cache");var catalog=CatalogService.Import(cp,cache,default);
  Ok("discover-three-synthetic-resources",catalog.Resources.Count==3);Ok("isolate-corrupt-resource",catalog.Resources.Count(r=>r.Status=="GEOMETRY_AVAILABLE")==2&&catalog.Resources[2].Status!="GEOMETRY_AVAILABLE");
  Ok("declared-layer-not-filename-derived",catalog.Resources[0].LayerChunks.SequenceEqual(new[]{"ACT6"})&&catalog.Resources[0].LayerMeaning.Contains("UNKNOWN"));
  Ok("variants-not-merged",catalog.Resources.Select(r=>r.Path).Distinct().Count()==3);
  var again=CatalogService.Import(cp,cache,default);Ok("cache-reopen",JsonSerializer.Serialize(catalog)==JsonSerializer.Serialize(again));
  var lazy=CatalogService.Load(cp,catalog,catalog.Resources[1],default);Ok("lazy-second-room",Math.Abs(lazy.Mesh.MinX-250)<.001);
  string cf=CatalogService.CachePath(cache,catalog.ImageSha256);var original=File.ReadAllBytes(cf);using(var cts=new CancellationTokenSource()){cts.Cancel();Reject("import-cancelled",()=>{try{CatalogService.Import(cp,cache,cts.Token);}catch(OperationCanceledException){throw new IOException("cancelled");}});}Ok("cancel-preserves-cache",File.ReadAllBytes(cf).SequenceEqual(original));
  File.WriteAllText(cf,"bad");var rebuilt=CatalogService.Import(cp,cache,default);Ok("corrupt-cache-rebuilt",rebuilt.Resources.Count==3);
  File.WriteAllText(cf,"{\"SchemaVersion\":1,\"Resources\":null}");Ok("null-cache-rebuilt",CatalogService.Import(cp,cache,default).Resources.Count==3);
  string cancelled=Path.Combine(work,"cancelled-cache");using(var cts=new CancellationTokenSource()){
   bool stopped=false;try{CatalogService.Import(cp,cancelled,cts.Token,new ProgressAction(s=>{if(s.StartsWith("Recurso 2/"))cts.Cancel();}));}catch(OperationCanceledException){stopped=true;}
   Ok("mid-import-cancellation-no-partial-cache",stopped&&!Directory.Exists(cancelled));
  }
  bad=ciso.ToArray();bad[32768+16]=1;File.WriteAllBytes(cp,bad);var changed=CatalogService.Import(cp,cache,default);Ok("changed-image-separate-cache-key",changed.ImageSha256!=catalog.ImageSha256&&File.Exists(cf));File.WriteAllBytes(cp,ciso);
  var tampered=rebuilt with{Resources=rebuilt.Resources.Select((r,i)=>i==0?r with{Path="../evil"}:r).ToList()};File.WriteAllText(cf,JsonSerializer.Serialize(tampered));Ok("cache-path-not-trusted",CatalogService.Import(cp,cache,default).Resources[0].Path==catalog.Resources[0].Path);
  bad=ciso.ToArray();bad[0x20000]^=1;File.WriteAllBytes(cp,bad);Reject("lazy-resource-hash-change",()=>CatalogService.Load(cp,catalog,catalog.Resources[2],default));File.WriteAllBytes(cp,ciso);
  var full=GeometryReader.Read(Archive.Read(File.ReadAllBytes(realArc)));var compat=Geometry.Load(realArc);Ok("authentic-room-full-mesh",full.Triangles.Count>=compat.Triangles.Count);
  double[][] points={new[]{-1334.75342,-6339.34229,60011.2109},new[]{-1805.60547,-6375.58301,60777.1289},new[]{-1039.68616,-6316.24902,61329.5977}};
  foreach(var p in points)Ok("CP08-human-reference-height-rounded-float32",Math.Abs(compat.Place((float)p[0],(float)p[2])-p[1])<.02);
  using(var bitmap=CatalogBrowser.Rasterize(full,3,default))Ok("real-room-raster-memory-only",bitmap.Width==1200);
  using(var ui=new CatalogBrowser(_=>{}))Ok("actual-catalog-WinForms",ui.Text.Contains("CP09"));
  File.WriteAllText(Path.Combine(output,"importer-result.json"),JsonSerializer.Serialize(new{status="CP09_IMPORTER_LAB_PASS",cases=pass.Count,case_names=pass,synthetic_resources=3,synthetic_geometry_rooms=2,authentic_rooms=1,authentic_arc_sha256=Geometry.Hash,authentic_triangles=full.Triangles.Count,authentic_bounds=new[]{full.MinX,full.MaxX,full.MinY,full.MaxY,full.MinZ,full.MaxZ},ciso_real="CISO_REAL_IMPORT_HUMAN_PENDING",additional_real_rooms="NOT_TESTED",game_executed=false},CatalogService.Options));
 }
}
