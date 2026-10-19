using System.Text;
using System.Text.Json;
using System.IO.Compression;
using System.Reflection;
namespace DevfMap;
internal static class Cp20Tests {
 internal static void Run(string matrix,string output){
  var pass=new List<string>();void Ok(string n,bool b){if(!b)throw new Exception(n);pass.Add(n);}
  bool Accept(byte[] raw){try{using var j=JsonDocument.Parse(raw);if(j.RootElement.GetProperty("schema_version").GetInt32()==1)Config.Decode(raw);else MultiConfig.Decode(raw);return true;}catch(Exception e)when(e is InvalidDataException or JsonException or ArgumentException or InvalidOperationException){return false;}}
  foreach(string line in File.ReadAllLines(Path.Combine(matrix,"matrix.tsv"))){var fields=line.Split('\t');var bytes=File.ReadAllBytes(Path.Combine(matrix,fields[0]+".json"));Ok("shared-matrix:"+fields[0],Accept(bytes)==(fields[1]=="1"));}
  foreach(string name in new[]{"legacy-absent","legacy-normal","existing-black","blue","red","black","mixed"}){
   byte[] original=File.ReadAllBytes(Path.Combine(matrix,name+".json"));var d=MultiConfig.Decode(original);var back=MultiConfig.Decode(MultiConfig.Encode(d));Ok("roundtrip:"+name,JsonSerializer.Serialize(d)==JsonSerializer.Serialize(back));
   string p=Path.Combine(output,"cp20-noop-"+name+".json");File.WriteAllBytes(p,original);var before=Directory.GetFiles(output).Length;var bak=MultiConfig.Save(p,d,MultiConfig.HashFile(p));Ok("exact-noop-save:"+name,bak==null&&File.ReadAllBytes(p).SequenceEqual(original)&&before==Directory.GetFiles(output).Length);
  }
  var mixed=MultiConfig.Decode(File.ReadAllBytes(Path.Combine(matrix,"mixed.json")));Ok("mixed-same-room-types",mixed.encounters.Select(e=>e.enemy).SequenceEqual(new[]{"E_OC2","E_OC","E_OC2","E_OC2"}));
  File.WriteAllBytes(Path.Combine(output,"cp20-mixed-export.json"),MultiConfig.Encode(mixed));
  string backup=Path.Combine(output,"cp20-explicit-conversion.backup.json");var bytes0=File.ReadAllBytes(Path.Combine(matrix,"legacy-normal.json"));VariantEditing.Backup(backup,bytes0);Ok("conversion-exact-backup",File.ReadAllBytes(backup).SequenceEqual(bytes0));
  var e0=MultiConfig.Decode(bytes0).encounters[0];foreach(string variant in new[]{"blue","red","black"}){
   var changed=VariantEditing.Change(e0,variant,variant=="black"?440:null);
   Ok("conversion-id-position-yaw:"+variant,changed.id==e0.id&&changed.x==e0.x&&changed.y==e0.y&&changed.z==e0.z&&changed.room==e0.room&&changed.layer==e0.layer&&changed.yaw_s16==e0.yaw_s16&&!changed.enabled&&!changed.qa_confirmed&&changed.layer_state=="QA_CANDIDATE");
  }
  var raw=(byte[])typeof(MultiTests).GetMethod("Synthetic",BindingFlags.NonPublic|BindingFlags.Static)!.Invoke(null,new object[]{1})!;
  var g=new PlacementGeometry(new(){Stage="F_SP121",Room=1,Variant="00",Path="res/Stage/F_SP121/R01_00.arc",Sha256=Disc.Hash(raw)},GeometryReader.Read(Archive.Read(raw)),raw);
  var doc=MultiDocument.Empty();doc.resources.Add(g.Permit);doc.encounters.Add(e0 with{resource=g.Permit.key,x=1000,y=g.Place(1000,1000),z=1000});
  using(var ui=new MultiEditor()){ui.TestVariantEditing(doc,g);Ok("actual-controls-blue-red-black-legacy-hp",true);ui.TestNewMixed(g);Ok("actual-add-three-variants-disabled-separated",true);ui.TestMarkerRendering(output);Ok("map-render-variant-colors-synthetic",true);}
  string mods=Path.Combine(output,"cp20-mods");Directory.CreateDirectory(mods);
  void Package(string file,string id,string version){using var zip=ZipFile.Open(Path.Combine(mods,file),ZipArchiveMode.Create);using var w=new StreamWriter(zip.CreateEntry("mod.json").Open());w.Write(JsonSerializer.Serialize(new{id,version}));}
  Package("provider.dusk","dev.devf.cp06_native_config_dual","0.14.0");Package("expansion.dusk","dev.bokoblin.expansion","0.16.3");VariantEditing.RequirePair(mods);Ok("install-compatible-pair",true);
  bool RejectPair(){try{VariantEditing.RequirePair(mods);return false;}catch(InvalidDataException){return true;}}
  Package("duplicate.dusk","dev.bokoblin.expansion","0.16.2");Ok("install-duplicate-expansion-rejected",RejectPair());File.Delete(Path.Combine(mods,"duplicate.dusk"));File.Delete(Path.Combine(mods,"expansion.dusk"));Package("expansion.dusk","dev.bokoblin.expansion","0.16.2");Ok("install-old-expansion-rejected",RejectPair());
  File.WriteAllText(Path.Combine(output,"cp20-result.json"),JsonSerializer.Serialize(new{status="CP20_EDITOR_LAB_PASS",cases=pass.Count,case_names=pass,game_executed=false},CatalogService.Options));
 }
}
