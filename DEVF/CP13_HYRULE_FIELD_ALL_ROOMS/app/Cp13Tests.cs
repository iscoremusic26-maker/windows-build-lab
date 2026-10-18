using System.Reflection;
using System.Text;
using System.Text.Json;
namespace DevfMap;
internal static class Cp13Tests{
 public static void Run(string arc,string output){
 var pass=new List<string>();void Ok(string n,bool v){if(!v)throw new Exception(n);pass.Add(n);}void Reject(string n,Action a){bool bad=false;try{a();}catch(Exception e)when(e is InvalidDataException or JsonException or IOException or ArgumentException){bad=true;}Ok(n,bad);}
 byte[] Raw(int room)=>(byte[])typeof(MultiTests).GetMethod("Synthetic",BindingFlags.NonPublic|BindingFlags.Static)!.Invoke(null,new object[]{room})!;
 PlacementGeometry Load(int room,string variant="00"){var raw=Raw(room);return new(new(){Stage="F_SP121",Room=room,Variant=variant,Path=$"res/Stage/F_SP121/R{room:00}_{variant}.arc",Sha256=Disc.Hash(raw)},GeometryReader.Read(Archive.Read(raw)),raw);}
 Ok("approved-new-black-default-440",MultiConfig.HpDefaultBlack==440);
 var d=MultiDocument.Empty();int[] rooms={0,1,3,5,6,7,9,10,15};var gs=rooms.Select(r=>Load(r)).ToArray();d.resources.AddRange(gs.Select(g=>g.Permit));
 for(int i=0;i<32;i++){var g=gs[i%gs.Length];float x=1000+(i/gs.Length)*400;var e=new MultiEncounter("cp13-"+i,true,"E_OC2",g.Permit.key,"F_SP121",g.Permit.room,6,x,g.Place(x,1000),1000,0,510,0xFFFF0100u,true,"QA_CANDIDATE",i%2==0?"black":null,i%2==0?440:null);g.Check(e);d.encounters.Add(e);}
 var encoded=MultiConfig.Encode(d);Ok("32-export-roundtrip",MultiConfig.Encode(MultiConfig.Decode(encoded)).SequenceEqual(encoded));Ok("larger-than-16KiB",encoded.Length>16384);File.WriteAllBytes(Path.Combine(output,"cp13-encounters.json"),encoded);
 foreach(int room in rooms)Ok("strict-synthetic-room-"+room,d.encounters.Any(e=>e.room==room));
 Reject("33-encounters",()=>MultiConfig.Validate(d with{encounters=d.encounters.Append(d.encounters[0] with{id="over-capacity"}).ToList()}));
 Reject("context-separation-300",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0],d.encounters[0] with{id="near",x=d.encounters[0].x+100}}}));
 MultiConfig.Validate(d with{encounters=new(){d.encounters[0],d.encounters[0] with{id="other-layer",layer=5}}});Ok("same-XYZ-other-layer",true);
 var variant=Load(0,"01");var variants=MultiDocument.Empty();variants.resources.AddRange(new[]{gs[0].Permit,variant.Permit});MultiConfig.Validate(variants);Ok("same-room-different-ARC-variant",true);
 var catalog=new GameCatalog{DiscId="GZ2E01",Revision=0,Resources=rooms.Select(r=>new ResourceInfo{Stage="F_SP121",Room=r,Variant="00",Path=$"res/Stage/F_SP121/R{r:00}_00.arc",Status=r==6?"KCL_DEGENERATE":"GEOMETRY_AVAILABLE"}).ToList()};
 catalog.Resources.Add(new(){Stage="F_SP121",Room=null,Variant="00",Path="res/Stage/F_SP121/STG_00.arc",Status="GEOMETRY_KCL_ABSENT_OR_MULTIPLE"});catalog.Resources.Add(new(){Stage="F_SP113",Room=8,Variant="00",Path="res/Stage/F_SP113/R08_00.arc"});
 Ok("catalog-all-real-entries-not-whitelist",Coverage.Rooms(catalog).Count()==rooms.Length);Ok("no-phantom-R08",!Coverage.Rooms(catalog).Any(r=>r.Room==8));Ok("STG-not-room",!Coverage.Rooms(catalog).Any(r=>r.Room==null));Ok("degenerate-remains-visible",Coverage.Rooms(catalog).Any(r=>r.Room==6&&r.Status=="KCL_DEGENERATE"));
 using(var ui=new MultiEditor()){ui.ObserveCatalog(catalog);Ok("actual-UI-all-entries",ui.ObservedCount==rooms.Length);ui.TestRoomSelection(d,gs[0],gs[5]);Ok("UI-room-switch-preserves-32",ui.ModelCount==32);}
 Reject("stage-alias-not-identity",()=>new PlacementGeometry(new(){Stage="Hyrule Field",Room=0,Variant="00",Path="res/Stage/F_SP121/R00_00.arc",Sha256=Disc.Hash(Raw(0))},gs[0].Mesh,Raw(0)));
 var noDzrFiles=Archive.Read(Raw(7)).Where(e=>!e.Key.EndsWith(".dzr",StringComparison.OrdinalIgnoreCase)).ToDictionary(e=>e.Key,e=>e.Value.ToArray());
 var noDzr=(byte[])typeof(ImporterTests).GetMethod("Rarc",BindingFlags.NonPublic|BindingFlags.Static)!.Invoke(null,new object[]{noDzrFiles})!;
 Reject("authentic-contract-requires-DZR",()=>new PlacementGeometry(new(){Stage="F_SP121",Room=7,Variant="00",Path="Stage/F_SP121/R07_00.arc",Sha256=Disc.Hash(noDzr)},GeometryReader.Read(Archive.Read(noDzr)),noDzr));
 Reject("wrong-PLC-room",()=>new PlacementGeometry(new(){Stage="F_SP121",Room=7,Variant="00",Path="res/Stage/F_SP121/R07_00.arc",Sha256=Disc.Hash(Raw(0))},gs[0].Mesh,Raw(0)));
 foreach(var l in new[]{-1,15})Reject("layer-domain-"+l,()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{layer=l}}}));
 Reject("legacy-not-for-room0",()=>gs[0].Check(d.encounters[0] with{layer_state="CP08_OBSERVED"}));
 string p=Path.Combine(output,"cp13-save.json");MultiConfig.Save(p,d,null);Ok("save-reopen-32",MultiConfig.Read(p).SequenceEqual(encoded));var off=d with{encounters=d.encounters.Select(e=>e with{enabled=false}).ToList()};var bak=MultiConfig.Save(p,off,MultiConfig.HashFile(p));Ok("backup-before-overwrite",bak!=null&&File.ReadAllBytes(bak).SequenceEqual(encoded));
 foreach(var phase in new[]{"backup","write","replace"}){var before=File.ReadAllBytes(p);Reject("write-fault-"+phase,()=>MultiConfig.Save(p,d,MultiConfig.HashFile(p),x=>{if(x==phase)throw new IOException("injected");}));Ok("write-recovery-"+phase,File.ReadAllBytes(p).SequenceEqual(before));}
 string prep=Path.Combine(output,"blocked.preparation.json");var blocked=catalog.Resources.First(r=>r.Room==6);Coverage.SavePreparation(prep,blocked,"Await local collision validation");using(var j=JsonDocument.Parse(File.ReadAllBytes(prep)))Ok("blocked-preparation-not-runtime",!j.RootElement.GetProperty("enabled").GetBoolean()&&j.RootElement.GetProperty("document_type").GetString()=="devf_blocked_preparation");Reject("preparation-rejected-by-runtime-schema",()=>MultiConfig.Decode(File.ReadAllBytes(prep)));
 Reject("128KiB-json-cap",()=>MultiConfig.Decode(Enumerable.Repeat((byte)' ',131073).ToArray()));
 File.WriteAllText(Path.Combine(output,"cp13-result.json"),JsonSerializer.Serialize(new{status="CP13_EDITOR_LAB_PASS",cases=pass.Count,case_names=pass,synthetic_rooms=rooms,authentic_rooms=new[]{3},geometry="CI_SYNTHETIC_PASS_EXCEPT_AUTHENTIC_R03",runtime="HUMAN_PENDING"},CatalogService.Options));
 }
}
