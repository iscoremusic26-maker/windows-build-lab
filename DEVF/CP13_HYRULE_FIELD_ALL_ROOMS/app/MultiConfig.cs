using System.Text.Json;
using System.Text.Json.Serialization;
namespace DevfMap;
public sealed record ResourcePermit(string key,string stage,int room,string variant,string path,string arc_sha256,string kcl_sha256,string plc_sha256,int plc_room,double[] bounds,string authorization);
// CP10.2: bokoblin_variant/hp are OPTIONAL and omitted from JSON when null, so documents without a black Bokoblin stay byte-identical to CP10.
// (The resource field "variant" is the ARC file variant suffix, unrelated to the Bokoblin variant.)
public sealed record MultiEncounter(string id,bool enabled,string enemy,string resource,string stage,int room,int layer,double x,double y,double z,int yaw_s16,int native_profile,uint parameters,bool qa_confirmed,string layer_state,[property:JsonIgnore(Condition=JsonIgnoreCondition.WhenWritingNull)] string? bokoblin_variant=null,[property:JsonIgnore(Condition=JsonIgnoreCondition.WhenWritingNull)] int? hp=null){
 [JsonIgnore] public bool IsBlack=>bokoblin_variant==MultiConfig.VariantBlack;
}
public sealed record MultiDocument(string document_type,int schema_version,string disc_id,int revision,List<ResourcePermit> resources,List<MultiEncounter> encounters){
 public static MultiDocument Empty()=>new("devf_map_encounters",2,"GZ2E01",0,new(),new());
}
public static class MultiConfig{
 public const int MaxEncounters=32,MaxResources=64,MaxBytes=131072;
 public const string VariantBlack="black",VariantNormal="normal";public const int HpMin=1,HpMax=999,HpDefaultBlack=440;
 public static void C(bool b,string reason)=>Disc.Require(b,reason);
 static bool Hash(string s)=>s!=null&&s.Length==64&&s.Any(c=>c!='0')&&s.All(c=>char.IsAsciiDigit(c)||c is >= 'a' and <= 'f');
 static bool Id(string s,int max=31)=>s!=null&&s.Length is >0&&s.Length<=max&&s.All(c=>c is >= 'a' and <= 'z'||char.IsAsciiDigit(c)||c is '_' or '-');
 public static void Keys(JsonElement o,params string[] names){C(o.ValueKind==JsonValueKind.Object,"OBJECT");var seen=new HashSet<string>();foreach(var p in o.EnumerateObject())C(names.Contains(p.Name)&&seen.Add(p.Name),"UNKNOWN_OR_DUPLICATE_KEY");C(seen.Count==names.Length,"MISSING_KEY");}
 // Required keys exactly once plus the listed optional keys at most once.
 public static void KeysOptional(JsonElement o,string[] required,string[] optional){C(o.ValueKind==JsonValueKind.Object,"OBJECT");var seen=new HashSet<string>();foreach(var p in o.EnumerateObject())C((required.Contains(p.Name)||optional.Contains(p.Name))&&seen.Add(p.Name),"UNKNOWN_OR_DUPLICATE_KEY");C(required.All(seen.Contains),"MISSING_KEY");}
 public static MultiDocument Decode(byte[] b){
 C(b.Length is >0 and <=MaxBytes&&b.All(x=>x<=127&&x!=92),"JSON_SIZE_ENCODING");using var j=JsonDocument.Parse(b,new JsonDocumentOptions{MaxDepth=8});var r=j.RootElement;
 Keys(r,"document_type","schema_version","disc_id","revision","resources","encounters");
 foreach(var p in r.GetProperty("resources").EnumerateArray())Keys(p,"key","stage","room","variant","path","arc_sha256","kcl_sha256","plc_sha256","plc_room","bounds","authorization");
 foreach(var p in r.GetProperty("encounters").EnumerateArray())KeysOptional(p,new[]{"id","enabled","enemy","resource","stage","room","layer","x","y","z","yaw_s16","native_profile","parameters","qa_confirmed","layer_state"},new[]{"bokoblin_variant","hp"});
 var d=JsonSerializer.Deserialize<MultiDocument>(b)??throw new InvalidDataException("JSON_NULL");Validate(d);return Canonical(d);
 }
 // "normal" is the absence of the field: one canonical form for Normal Bokoblins.
 public static MultiDocument Canonical(MultiDocument d)=>d.encounters.Any(e=>e.bokoblin_variant==VariantNormal)?d with{encounters=d.encounters.Select(e=>e.bokoblin_variant==VariantNormal?e with{bokoblin_variant=null}:e).ToList()}:d;
 public static byte[] Encode(MultiDocument d){Validate(d);var b=JsonSerializer.SerializeToUtf8Bytes(Canonical(d),new JsonSerializerOptions{WriteIndented=true});Decode(b);return b;}
 public static void Validate(MultiDocument d){
 C(d.document_type=="devf_map_encounters"&&d.schema_version==2&&d.disc_id=="GZ2E01"&&d.revision==0,"SCHEMA_DISC");
 C(d.resources!=null&&d.encounters!=null&&d.resources.Count<=MaxResources&&d.encounters.Count<=MaxEncounters,"CAPACITY");
 var keys=new HashSet<string>();var rooms=new HashSet<(int,string)>();var hashes=new HashSet<string>();
 foreach(var r in d.resources){
 C(Id(r.key,32)&&keys.Add(r.key)&&rooms.Add((r.room,r.variant)),"RESOURCE_DUPLICATE");
 C(r.stage=="F_SP121"&&r.room is >=0 and <=63&&r.variant.Length==2&&r.variant.All(char.IsAsciiDigit)&&r.path==$"Stage/F_SP121/R{r.room:00}_{r.variant}.arc"&&r.plc_room==r.room,"RESOURCE_SCENE");
 C(Hash(r.arc_sha256)&&Hash(r.kcl_sha256)&&Hash(r.plc_sha256)&&r.authorization=="STATIC_QA_NOT_RUNTIME_VERIFIED","RESOURCE_PROVENANCE");
 C(r.bounds!=null&&r.bounds.Length==6&&r.bounds.All(x=>double.IsFinite(x)&&Math.Abs(x)<=1000000)&&r.bounds[0]<r.bounds[1]&&r.bounds[2]<=r.bounds[3]&&r.bounds[4]<r.bounds[5],"RESOURCE_BOUNDS");
 }
 var ids=new HashSet<string>();var prior=new List<MultiEncounter>();
 foreach(var e in d.encounters){
 C(e.bokoblin_variant==null||e.bokoblin_variant==VariantNormal||e.bokoblin_variant==VariantBlack,"VARIANT_UNKNOWN");
 C(e.IsBlack?e.hp is >=HpMin and <=HpMax:e.hp==null,"VARIANT_HP");
 C(Id(e.id)&&ids.Add(e.id)&&e.enemy=="E_OC2"&&e.stage=="F_SP121"&&e.room is >=0 and <=63&&e.layer is >=0 and <=14&&e.yaw_s16 is >=-32768 and <=32767&&e.native_profile==510&&e.parameters==0xFFFF0100u,"ENCOUNTER_FIELDS");
 var r=d.resources.SingleOrDefault(r=>r.key==e.resource);C(r!=null&&r.room==e.room,"RESOURCE_REFERENCE");
 var b=r!.bounds;C(double.IsFinite(e.x)&&double.IsFinite(e.y)&&double.IsFinite(e.z)&&e.x>=b[0]+100&&e.x<=b[1]-100&&e.y>=b[2]+5-.02&&e.y<=b[3]+5+.02&&e.z>=b[4]+100&&e.z<=b[5]-100,"POSITION_BOUNDS");
 if(e.layer_state=="CP08_OBSERVED")C(e.room==3&&e.layer==6&&r.arc_sha256==Geometry.Hash&&e.x>=-2150&&e.x<=-350&&e.z>=59100&&e.z<=62900,"LEGACY_IDENTITY");
 else C(e.layer_state=="QA_CANDIDATE"&&(!e.enabled||e.qa_confirmed),"QA_NOT_CONFIRMED");
 foreach(var a in prior)if(a.room==e.room&&a.layer==e.layer){C(!((float)a.x==(float)e.x&&(float)a.y==(float)e.y&&(float)a.z==(float)e.z),"DUPLICATE_POSITION");C(!a.enabled||!e.enabled||Math.Sqrt(Math.Pow((double)(float)a.x-(float)e.x,2)+Math.Pow((double)(float)a.z-(float)e.z,2))>=300,"SEPARATION_300");}
 prior.Add(e);
 }
 }
 public static byte[] Read(string path){using var f=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read);C(f.Length>0&&f.Length<=MaxBytes,"JSON_SIZE");var b=new byte[(int)f.Length];f.ReadExactly(b);return b;}
 public static string? HashFile(string p)=>File.Exists(p)?Disc.Hash(Read(p)):null;
 // Atomic rename on this filesystem is not a power-loss guarantee. Verified backup survives errors.
 public static string? Save(string path,MultiDocument d,string? expected,Action<string>? fault=null){
 byte[] bytes=Encode(d);C(HashFile(path)==expected,"EXTERNAL_FILE_CHANGED");byte[]? old=expected==null?null:Read(path);string? backup=null;
 if(old!=null){using var j=JsonDocument.Parse(old);if(j.RootElement.GetProperty("schema_version").GetInt32()==1)Config.Decode(old);else Decode(old);backup=path+".backup-"+Guid.NewGuid().ToString("N");using(var s=new FileStream(backup,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(old);s.Flush(true);}C(File.ReadAllBytes(backup).SequenceEqual(old),"BACKUP_VERIFY");}
 fault?.Invoke("backup");string temp=path+".tmp-"+Guid.NewGuid().ToString("N");bool replaced=false;
 try{using(var s=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(bytes);s.Flush(true);}fault?.Invoke("write");C(File.ReadAllBytes(temp).SequenceEqual(bytes),"TEMP_VERIFY");Decode(File.ReadAllBytes(temp));C(HashFile(path)==expected,"EXTERNAL_FILE_CHANGED");File.Move(temp,path,true);replaced=true;fault?.Invoke("replace");C(File.ReadAllBytes(path).SequenceEqual(bytes),"SAVED_VERIFY");Decode(File.ReadAllBytes(path));return backup;}
 catch{if(replaced){if(old!=null){File.WriteAllBytes(path,old);C(File.ReadAllBytes(path).SequenceEqual(old),"ROLLBACK_FAILED_BACKUP_RETAINED");}else if(File.Exists(path))File.Delete(path);}throw;}
 finally{if(File.Exists(temp))File.Delete(temp);}
 }
}
