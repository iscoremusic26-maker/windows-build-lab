using System.Text;
using System.Text.Json;
using System.Reflection;
using System.Buffers.Binary;
namespace DevfMap;
internal static class MultiTests{
 static byte[] Synthetic(int room){
 var raw=(byte[])typeof(ImporterTests).GetMethod("Room",BindingFlags.NonPublic|BindingFlags.Static)!.Invoke(null,new object[]{room,0f})!;
 var files=Archive.Read(raw).ToDictionary(e=>e.Key,e=>e.Value.ToArray());
 BinaryPrimitives.WriteInt32BigEndian(files["room.kcl"].AsSpan(132,4),BitConverter.SingleToInt32Bits(7071.0678f));
 return (byte[])typeof(ImporterTests).GetMethod("Rarc",BindingFlags.NonPublic|BindingFlags.Static)!.Invoke(null,new object[]{files})!;
 }
 public static void Run(string arc,string output){
 var pass=new List<string>();void Ok(string n,bool v){if(!v)throw new Exception(n);pass.Add(n);}
 void Reject(string n,Action a){bool bad=false;try{a();}catch(Exception e)when(e is InvalidDataException or IOException or JsonException or ArgumentException){bad=true;}Ok(n,bad);}
 PlacementGeometry Load(int room){var raw=Synthetic(room);var mesh=GeometryReader.Read(Archive.Read(raw));return new(new ResourceInfo{Stage="F_SP121",Room=room,Variant="00",Path=$"res/Stage/F_SP121/R{room:00}_00.arc",Sha256=Disc.Hash(raw)},mesh,raw);}
 var a=Load(1);var b=Load(2);var d=MultiDocument.Empty();d.resources.AddRange(new[]{a.Permit,b.Permit});
 MultiEncounter Item(string id,PlacementGeometry g,int layer,float x=1000,float z=1000,bool on=true)=>new(id,on,"E_OC2",g.Permit.key,"F_SP121",g.Permit.room,layer,x,g.Place(x,z),z,123,510,0xFFFF0100u,on,"QA_CANDIDATE");
 d.encounters.Add(Item("room-one",a,2));d.encounters.Add(Item("room-two",b,9));
 var bytes=MultiConfig.Encode(d);Ok("synthetic-two-rooms-roundtrip",MultiConfig.Encode(MultiConfig.Decode(bytes)).SequenceEqual(bytes));
 Ok("cross-room-same-XZ",d.encounters[0].x==d.encounters[1].x);Ok("floor-plus-five",a.Place(1000,1000)==5);
 Reject("PLC-wrong-room",()=>new PlacementGeometry(a.Permit with{room=2},a.Mesh).Place(1000,1000));
 var t=a.Mesh.Triangles[0];var upper=t with{P=t.P+new V(0,60,0),Q=t.Q+new V(0,60,0),R=t.R+new V(0,60,0)};
 Reject("ambiguous-floors",()=>new PlacementGeometry(a.Permit,a.Mesh with{Triangles=new(){t,upper}}).Place(1000,1000));Reject("footprint-edge",()=>a.Place(20,20));
 Reject("wall-screen",()=>new PlacementGeometry(a.Permit,a.Mesh with{Triangles=new(){t,t with{P=new(1000,10,900),Q=new(1000,100,1100),R=new(1000,10,1100),N=new(1,0,0)}}}).Place(1000,1000));
 var serialized=Encoding.UTF8.GetString(bytes);
 foreach(var pair in new[]{("\"room-one\"","\"room-two\""),("\"GZ2E01\"","\"GZ2P01\""),("\"schema_version\": 2","\"schema_version\": 3"),("\"F_SP121\"","\"F_SP113\""),("\"variant\": \"00\"","\"variant\": \"01\""),("\"layer\": 2","\"layer\": -1"),("\"layer\": 2","\"layer\": 15"),("\"qa_confirmed\": true","\"qa_confirmed\": false"),("\"native_profile\": 510","\"native_profile\": 511"),("\"parameters\": 4294902016","\"parameters\": 0"),("\"x\": 1000","\"x\": 1e999"),("\"yaw_s16\": 123","\"yaw_s16\": 40000"),("\"resource\": \"r01-00\"","\"resource\": \"missing\""),("\"room\": 1","\"room\": 6"),("\"plc_room\": 1","\"plc_room\": 2"),("\"path\": \"Stage/F_SP121/R01_00.arc\"","\"path\": \"../R01_00.arc\"")})Reject("schema-negative-"+pair.Item1,()=>MultiConfig.Decode(Encoding.UTF8.GetBytes(serialized.Replace(pair.Item1,pair.Item2))));
 Reject("duplicate-key",()=>MultiConfig.Decode(Encoding.UTF8.GetBytes(serialized.Replace("\"revision\": 0","\"revision\": 0,\"revision\":0"))));
 Reject("unknown-key",()=>MultiConfig.Decode(Encoding.UTF8.GetBytes(serialized.Replace("\"revision\": 0","\"revision\": 0,\"surprise\":0"))));Reject("BOM",()=>MultiConfig.Decode(new byte[]{239,187,191}.Concat(bytes).ToArray()));
 Reject("same-room-separation",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0],Item("near",a,2,1100,1000)}}));
 MultiConfig.Validate(d with{encounters=new(){d.encounters[0],Item("other-layer",a,4)}});Ok("cross-layer-context",true);
 var eight=d with{encounters=Enumerable.Range(0,8).Select(i=>Item("eight-"+i,a,2,1000+i*400,1000)).ToList()};MultiConfig.Validate(eight);Ok("eight-max",true);
 Reject("nine-total",()=>MultiConfig.Validate(eight with{encounters=eight.encounters.Append(Item("ninth",a,2,5000,1000,false)).ToList()}));
 var disabled=d with{encounters=d.encounters.Select(e=>e with{enabled=false,qa_confirmed=false}).ToList()};MultiConfig.Validate(disabled);Ok("disabled-no-consent",true);
 string path=Path.Combine(output,"multi-safe.json");MultiConfig.Save(path,d,null);string h=MultiConfig.HashFile(path)!;Ok("save-reopen-exact",File.ReadAllBytes(path).SequenceEqual(bytes));
 string? backup=MultiConfig.Save(path,disabled,h);Ok("backup-verified",backup!=null&&File.ReadAllBytes(backup).SequenceEqual(bytes));Reject("external-conflict",()=>MultiConfig.Save(path,d,h));
 foreach(var phase in new[]{"backup","write","replace"}){byte[] original=File.ReadAllBytes(path);Reject("fault-"+phase,()=>MultiConfig.Save(path,d,MultiConfig.HashFile(path),s=>{if(s==phase)throw new IOException("test");}));Ok("recovery-"+phase,File.ReadAllBytes(path).SequenceEqual(original));}
 var raw=File.ReadAllBytes(arc);var mesh=GeometryReader.Read(Archive.Read(raw));var real=new PlacementGeometry(new(){Stage="F_SP121",Room=3,Variant="00",Path="res/Stage/F_SP121/R03_00.arc",Sha256=Geometry.Hash},mesh,raw);
 var originalLegacy=File.ReadAllBytes(Path.Combine(output,"encounters.json"));var legacy=Config.Decode(originalLegacy);var migrated=MultiEditor.ConvertLegacy(legacy,real);
 Ok("legacy-source-unchanged",originalLegacy.SequenceEqual(File.ReadAllBytes(Path.Combine(output,"encounters.json"))));
 Ok("legacy-exact-XYZ",migrated.encounters.Zip(legacy.encounters).All(p=>p.First.x==p.Second.x&&p.First.y==p.Second.y&&p.First.z==p.Second.z&&p.First.enabled==p.Second.enabled&&p.First.id==p.Second.id&&p.First.stage==p.Second.stage&&p.First.room==p.Second.room&&p.First.layer==p.Second.layer&&p.First.yaw_s16==p.Second.yaw_s16));

 var human=Document.Empty();int num=0;
 foreach(var p in new[]{(-1334.75342,-6339.34229,60011.2109),(-1805.60547,-6375.58301,60777.1289),(-1039.68616,-6316.24902,61329.5977)})
 human.encounters.Add(new("bokoblin-"+(++num).ToString("000"),true,"E_OC2","F_SP121",3,6,p.Item1,p.Item2,p.Item3,0));
 var humanBefore=Config.Encode(human);var copied=MultiEditor.ConvertLegacy(human,real);
 Ok("three-human-vectors-original-XYZ",copied.encounters.Zip(human.encounters).All(p=>p.First.x==p.Second.x&&p.First.y==p.Second.y&&p.First.z==p.Second.z));
 Ok("schema1-bytes-not-mutated",humanBefore.SequenceEqual(Config.Encode(human)));
 Ok("authentic-R03-5326-triangles",mesh.Triangles.Count==5326&&Disc.Hash(raw)==Geometry.Hash);
 foreach(var e in copied.encounters){real.Check(e);Ok("authentic-legacy-height-original-validator",true);}
 Reject("legacy-context-wrong-layer",()=>real.Check(copied.encounters[0] with{layer=2}));
 Reject("legacy-context-wrong-stage",()=>real.Check(copied.encounters[0] with{stage="F_SP113"}));
 Reject("legacy-height-mismatch",()=>real.Check(copied.encounters[0] with{y=copied.encounters[0].y+10}));
 Reject("legacy-state-not-authority-other-resource",()=>a.Check(d.encounters[0] with{layer_state="CP08_OBSERVED"}));
 Reject("legacy-hash-without-authentic-reader",()=>new PlacementGeometry(real.Permit,mesh).Check(copied.encounters[0]));
 int ambiguous=0;foreach(var e in migrated.encounters){try{real.Check(e with{layer_state="QA_CANDIDATE"});}catch(InvalidDataException ex){if(ex.Message=="FLOOR_AMBIGUOUS")ambiguous++;else throw;}}
 Ok("new-QA-room3-does-not-use-legacy-bypass",ambiguous>0);
 using(var roomUi=new MultiEditor()){roomUi.TestRoomSelection(migrated,real,a);Ok("room-selection-does-not-mutate-legacy",true);}

 migrated.resources.AddRange(d.resources);migrated.encounters.AddRange(d.encounters);MultiConfig.Validate(migrated);File.WriteAllBytes(Path.Combine(output,"multi-encounters.json"),MultiConfig.Encode(migrated));
 using(var ui=new MultiEditor()){Ok("actual-WinForms",ui.Text.Contains("CP10")&&ui.ModelCount==0);ui.Accept(new(){Stage="F_SP121",Room=3,Variant="00",Path="res/Stage/F_SP121/R03_00.arc",Sha256=Geometry.Hash},mesh,raw);Ok("UI-resource-load",true);}
 var projection=new Projection{CenterX=1000,CenterZ=1000,Scale=.5};var round=projection.World(projection.Screen(1100,1400,new(800,600)),new(800,600));Ok("map-world-roundtrip",round.X==1100&&round.Z==1400);
 File.WriteAllText(Path.Combine(output,"multi-result.json"),JsonSerializer.Serialize(new{status="CP10_STATIC_LAB_PASS",cases=pass.Count,case_names=pass,synthetic_rooms=2,authentic_rooms=1,new_room_runtime="HUMAN_RUNTIME_PENDING"},CatalogService.Options));
 }
}
