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
 Reject("33-total",()=>MultiConfig.Validate(eight with{encounters=Enumerable.Repeat(eight.encounters[0],33).ToList()}));
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
 using(var roomUi2=new MultiEditor()){roomUi2.TestLabelCollision(real,a);Ok("label-collision-selects-by-key",true);}
 var rawA=Synthetic(1);var meshA=GeometryReader.Read(Archive.Read(rawA));
 Ok("name-exists-for-ineligible-resource",NameResolver.Resolve("F_SP113",1,"00").StageName=="Zora's Domain");
 Reject("named-resource-not-eligible",()=>new PlacementGeometry(new ResourceInfo{Stage="F_SP113",Room=1,Variant="00",Path="res/Stage/F_SP113/R01_00.arc",Sha256=Disc.Hash(rawA)},meshA,rawA));
 Reject("named-sacred-grove-not-eligible",()=>new PlacementGeometry(new ResourceInfo{Stage="F_SP117",Room=1,Variant="00",Path="res/Stage/F_SP117/R01_00.arc",Sha256=Disc.Hash(rawA)},meshA,rawA));

 copied.resources.AddRange(d.resources);copied.encounters.AddRange(d.encounters);MultiConfig.Validate(copied);Ok("three-legacy-plus-two-synthetic-within-capacity",copied.encounters.Count==5);File.WriteAllBytes(Path.Combine(output,"multi-encounters.json"),MultiConfig.Encode(copied));
 using(var ui=new MultiEditor()){Ok("actual-WinForms",ui.Text.Contains("CP20")&&ui.ModelCount==0);ui.Accept(new(){Stage="F_SP121",Room=3,Variant="00",Path="res/Stage/F_SP121/R03_00.arc",Sha256=Geometry.Hash},mesh,raw);Ok("UI-resource-load",true);}
 var projection=new Projection{CenterX=1000,CenterZ=1000,Scale=.5};var round=projection.World(projection.Screen(1100,1400,new(800,600)),new(800,600));Ok("map-world-roundtrip",round.X==1100&&round.Z==1400);
 {string json=Encoding.UTF8.GetString(MultiConfig.Encode(copied));
  Ok("labels-not-in-json",!new[]{"Hyrule Field","Sacred Grove","Zora","Room 0","Room 1","(inferred)","Stage F_","variant "}.Any(s=>json.Contains(s)));
  var reopened=MultiConfig.Decode(Encoding.UTF8.GetBytes(json));
  Ok("schema2-canonical-roundtrip-after-names",JsonSerializer.Serialize(reopened)==JsonSerializer.Serialize(copied)&&MultiConfig.Encode(reopened).SequenceEqual(Encoding.UTF8.GetBytes(json)));
  Ok("schema2-ids-stages-rooms-layers-flags-xyz-exact",reopened.encounters.Zip(copied.encounters).All(p=>p.First==p.Second));}
 {
  // ---- CP10.2 Black variant: optional fields, backward compatible, strict ----
  var plain=MultiConfig.Encode(d);string plainJson=Encoding.UTF8.GetString(plain);
  Ok("normal-document-has-no-variant-keys",!plainJson.Contains("bokoblin_variant")&&!plainJson.Contains("\"hp\""));
  var blackEnc=d.encounters[0] with{bokoblin_variant="black",hp=150};
  var bd=d with{encounters=new(){blackEnc,d.encounters[1]}};
  var blackBytes=MultiConfig.Encode(bd);string blackJson=Encoding.UTF8.GetString(blackBytes);
  Ok("black-serialized-explicitly-once",blackJson.Contains("\"bokoblin_variant\": \"black\"")&&blackJson.Contains("\"hp\": 150")&&blackJson.Split("bokoblin_variant").Length==2);
  var back=MultiConfig.Decode(blackBytes);
  Ok("black-roundtrip-exact",back.encounters[0]==blackEnc&&back.encounters[1]==d.encounters[1]&&back.encounters[1].bokoblin_variant==null&&back.encounters[1].hp==null&&MultiConfig.Encode(back).SequenceEqual(blackBytes));
  Ok("legacy-without-variant-is-normal",MultiConfig.Decode(plain).encounters.All(e=>!e.IsBlack&&e.bokoblin_variant==null&&e.hp==null));
  Ok("black-only-changes-variant-fields",(back.encounters[0] with{bokoblin_variant=null,hp=null})==d.encounters[0]);
  string explicitNormal=plainJson.Replace("\"layer_state\": \"QA_CANDIDATE\"","\"layer_state\": \"QA_CANDIDATE\",\n      \"bokoblin_variant\": \"normal\"");
  Ok("explicit-normal-preserved-without-migration",explicitNormal!=plainJson&&MultiConfig.Decode(Encoding.UTF8.GetBytes(explicitNormal)).encounters.All(e=>e.bokoblin_variant=="normal"));
  Reject("black-without-hp",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{bokoblin_variant="black"}}}));
  Reject("hp-without-variant",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{hp=150}}}));
  Reject("normal-with-hp",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{bokoblin_variant="normal",hp=150}}}));
  Reject("unknown-variant",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{bokoblin_variant="blue",hp=150}}}));
  Reject("hp-zero",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{bokoblin_variant="black",hp=0}}}));
  Reject("hp-over-999",()=>MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{bokoblin_variant="black",hp=1000}}}));
  foreach(var edge in new[]{1,999})MultiConfig.Validate(d with{encounters=new(){d.encounters[0] with{bokoblin_variant="black",hp=edge}}});Ok("hp-bounds-1-and-999",true);
  Reject("variant-key-twice",()=>MultiConfig.Decode(Encoding.UTF8.GetBytes(blackJson.Replace("\"hp\": 150","\"hp\": 150,\n      \"hp\": 150"))));
  Reject("variant-json-wrong-type",()=>MultiConfig.Decode(Encoding.UTF8.GetBytes(blackJson.Replace("\"hp\": 150","\"hp\": \"150\""))));
  var legacyText=Encoding.UTF8.GetString(originalLegacy);int yawAt=legacyText.IndexOf("\"yaw_s16\"",StringComparison.Ordinal);
  Reject("schema1-rejects-variant-keys",()=>Config.Decode(Encoding.UTF8.GetBytes(legacyText.Insert(yawAt,"\"bokoblin_variant\": \"black\", "))));
  Ok("black-keeps-same-position-rules",new Func<bool>(()=>{try{MultiConfig.Validate(bd with{encounters=new(){blackEnc,Item("too-close",a,2,1100,1000,true) with{bokoblin_variant="black",hp=150}}});return false;}catch(InvalidDataException){return true;}})());
  var legacyBlack=copied with{encounters=copied.encounters.Select((e,i)=>i==0?e with{bokoblin_variant="black",hp=150}:e).ToList()};
  MultiConfig.Validate(legacyBlack);real.Check(legacyBlack.encounters[0]);Ok("legacy-position-may-be-black-without-FLOOR_AMBIGUOUS",legacyBlack.encounters[0].layer_state=="CP08_OBSERVED"&&legacyBlack.encounters[0].IsBlack);
  using(var vui=new MultiEditor()){vui.TestVariantEditing(d,a);Ok("UI-variant-editing-rules",true);}
  File.WriteAllBytes(Path.Combine(output,"multi-encounters-black.json"),MultiConfig.Encode(legacyBlack)); // consumed by the C++ parser in CI
 }
 foreach(var n in NameTests.Run(output))pass.Add("names:"+n);
 File.WriteAllText(Path.Combine(output,"multi-result.json"),JsonSerializer.Serialize(new{status="CP10_STATIC_LAB_PASS",cases=pass.Count,case_names=pass,synthetic_rooms=2,authentic_rooms=1,new_room_runtime="HUMAN_RUNTIME_PENDING"},CatalogService.Options));
 }
}
