using System.Text.Json;
using System.Text.RegularExpressions;
namespace DevfMap;
public static class Coverage{
 public static string Key(ResourceInfo r)=>$"r{r.Room:00}-{r.Variant}";
 public static IEnumerable<ResourceInfo> Rooms(GameCatalog c)=>c.Resources.Where(r=>r.Stage=="F_SP121"&&r.Room is >=0 and <=63&&Regex.IsMatch(r.Variant,"^[0-9]{2}$")&&(r.Path==$"Stage/F_SP121/R{r.Room:00}_{r.Variant}.arc"||r.Path.EndsWith($"/Stage/F_SP121/R{r.Room:00}_{r.Variant}.arc",StringComparison.Ordinal))).OrderBy(r=>r.Room).ThenBy(r=>r.Variant);
 public static string Reason(ResourceInfo r)=>$"{r.Stage}/R{r.Room:00}_{r.Variant}: {r.Status}. "+(r.Status=="GEOMETRY_AVAILABLE"?"Candidata: exige PLC room correcto, DZR, huella, pendiente, paredes, hash y capa explícita; runtime pendiente.":r.Status=="KCL_DEGENERATE"?"El lector encontró un prisma con denominador de reconstrucción degenerado. No omitir triángulos ni certificar suelo. Diagnosticar recurso local; no hay asociación/transformación a STG demostrada.":"No hay malla KCL/PLC única reconstruible de este recurso. STG u otra sala no se heredan. Se necesita fuente de colisión auténtica con vínculo y transformación comprobados; mantener activación bloqueada.");
 public static object Report(string image,GameCatalog c,CancellationToken token){
  Disc.Require(c.DiscId=="GZ2E01"&&c.Revision==0,"DISC");var rows=new List<object>();
  foreach(var r in c.Resources.Where(r=>r.Stage=="F_SP121").OrderBy(r=>r.Room).ThenBy(r=>r.Variant)){
   token.ThrowIfCancellationRequested();int? kcl=null,plc=null,dzr=null;string diagnostic="NOT_READ";
   try{var raw=CatalogService.ReadResource(image,c,r,token);var members=Archive.Read(raw,token);kcl=members.Keys.Count(k=>k.EndsWith(".kcl",StringComparison.OrdinalIgnoreCase));plc=members.Keys.Count(k=>k.EndsWith(".plc",StringComparison.OrdinalIgnoreCase));dzr=members.Keys.Count(k=>k.EndsWith(".dzr",StringComparison.OrdinalIgnoreCase));try{var m=GeometryReader.Read(members,token);diagnostic="RECONSTRUCTED_LOCAL_NOT_RUNTIME_VERIFIED";}catch(InvalidDataException e){diagnostic=e.Message;}}
   catch(OperationCanceledException){throw;}catch(InvalidDataException e){diagnostic=e.Message;}catch{diagnostic="LOCAL_RESOURCE_UNAVAILABLE";}
   rows.Add(new{stage=r.Stage,room=r.Room,variant=r.Variant,resource=r.Path,kind=r.Room.HasValue?"ROOM_RESOURCE":"STAGE_RESOURCE_NOT_A_ROOM",geometry_status=r.Status,local_diagnostic=diagnostic,kcl_members=kcl,plc_members=plc,dzr_members=dzr,plc_rooms=r.PlcRooms,triangles=r.Triangles,arc_sha256=r.Sha256,kcl_sha256=r.KclHash,plc_sha256=r.PlcHash,selectable=Rooms(c).Any(x=>ReferenceEquals(x,r)),placement=r.Room.HasValue&&r.Status=="GEOMETRY_AVAILABLE"?"STRICT_LOCAL_CHECK_REQUIRED":"PLACEMENT_BLOCKED_PENDING_VALIDATION",layer="EXPLICIT_QA_0_14_NOT_INFERRED",human="NOT_VERIFIED_BY_CP13",reason=Reason(r)});
  }
  return new{document_type="devf_cp13_coverage_metadata",schema_version=1,disc_id="GZ2E01",revision=0,rooms=Rooms(c).Count(),entries=rows};
 }
 public static void SavePreparation(string path,ResourceInfo r,string note){
  Disc.Require(r.Stage=="F_SP121"&&r.Room is >=0 and <=63&&note.Length<=512,"PREPARATION_FIELDS");
  var b=JsonSerializer.SerializeToUtf8Bytes(new{document_type="devf_blocked_preparation",schema_version=1,stage=r.Stage,room=r.Room,variant=r.Variant,resource=r.Path,geometry_status=r.Status,state="PLACEMENT_BLOCKED_PENDING_VALIDATION",enabled=false,note},CatalogService.Options);
  string? backup=null;if(File.Exists(path)){Disc.Require(new FileInfo(path).Length<=16384,"PREPARATION_SIZE");var old=File.ReadAllBytes(path);using var j=JsonDocument.Parse(old);Disc.Require(j.RootElement.GetProperty("document_type").GetString()=="devf_blocked_preparation","PREPARATION_TYPE");backup=path+".backup-"+Guid.NewGuid().ToString("N");using(var s=new FileStream(backup,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(old);s.Flush(true);}Disc.Require(File.ReadAllBytes(backup).SequenceEqual(old),"BACKUP_VERIFY");}
  string temp=path+".tmp-"+Guid.NewGuid().ToString("N");try{using(var s=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(b);s.Flush(true);}Disc.Require(File.ReadAllBytes(temp).SequenceEqual(b),"PREPARATION_VERIFY");File.Move(temp,path,true);}finally{if(File.Exists(temp))File.Delete(temp);}
 }
}
