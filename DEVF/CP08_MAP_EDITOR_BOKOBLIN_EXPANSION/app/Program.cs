using System.Text.Json;
namespace DevfMap;
internal static class Program {
 [STAThread] static int Main(string[] args){
  if(args.Length==3&&args[0]=="--self-test"){try{Tests.Run(args[1],args[2]);return 0;}catch(Exception e){Directory.CreateDirectory(args[2]);File.WriteAllText(Path.Combine(args[2],"FAIL.txt"),e.ToString());return 1;}}
  ApplicationConfiguration.Initialize();Application.Run(new Editor());return 0;
 }
}
internal static class Tests {
 public static void Run(string arc,string output){
  Directory.CreateDirectory(output);var passed=new List<string>();void Check(string n,bool b){if(!b)throw new Exception(n);passed.Add(n);}
  var g=Geometry.Load(arc);Check("real-KCL-import",g.Triangles.Count>0);Check("known-ground-reader-agrees",Math.Abs(g.Ground(-1250,61950)-(-6327.600450915229))<.001);
  var projection=new Projection();for(int i=0;i<20;i++){var p=projection.Screen(-2000+i*50,59600+i*100,new Size(900,600));var w=projection.World(p,new Size(900,600));Check("world-map-roundtrip-"+i,Math.Abs(w.X-(-2000+i*50))<.01&&Math.Abs(w.Z-(59600+i*100))<.01);}
  var d=Document.Empty();var candidates=new List<object>();
  for(float z=59400;z<=62600&&d.encounters.Count<8;z+=400)for(float x=-1950;x<=-550&&d.encounters.Count<8;x+=400){try{float y=g.Place(x,z);var e=new Encounter("bokoblin-"+(d.encounters.Count+1).ToString("000"),true,"E_OC2","F_SP121",3,6,x,y,z,-2421);var next=d with{encounters=d.encounters.Append(e).ToList()};Config.Validate(next);d=next;candidates.Add(new{x,y,z});}catch(InvalidDataException){}}
  Check("at-least-three-new-static-supported-sites",d.encounters.Count>=3);Config.GeometryCheck(d,g);
  var encoded=Config.Encode(d);Check("roundtrip-exact",Config.Encode(Config.Decode(encoded)).SequenceEqual(encoded));
  string file=Path.Combine(output,"encounters.json");if(File.Exists(file))File.Delete(file);Config.Save(file,d,null);Check("reopen",Config.Decode(File.ReadAllBytes(file)).encounters.Count==d.encounters.Count);
  var none=d with{encounters=d.encounters.Select(e=>e with{enabled=false}).ToList()};
  foreach(string fault in new[]{"backup","write","flush","close","verify"}){bool caught=false;var before=File.ReadAllBytes(file);try{Config.Save(file,none,Config.Hash(before),p=>{if(p==fault)throw new IOException("injected");});}catch(IOException){caught=true;}Check("write-recovery-"+fault,caught&&File.ReadAllBytes(file).SequenceEqual(before));}
  string oversized=Path.Combine(output,"oversized.json");File.WriteAllBytes(oversized,new byte[16385]);bool large=false;try{Config.Read(oversized);}catch(InvalidDataException){large=true;}Check("bounded-file-read-before-allocation",large);
  bool conflict=false;try{Config.Save(file,none,"different");}catch(IOException){conflict=true;}Check("external-conflict-no-overwrite",conflict);
  var backup=Config.Save(file,none,Config.Hash(File.ReadAllBytes(file)));Check("verified-backup",File.Exists(backup)&&File.ReadAllBytes(backup).SequenceEqual(encoded));
  Config.Save(file,d,Config.Hash(File.ReadAllBytes(file)));Check("restore-original",File.ReadAllBytes(file).SequenceEqual(encoded));
  foreach(var bad in new[]{d with{schema_version=2},d with{encounters=d.encounters.Append(d.encounters[0]).ToList()},d with{encounters=new(){d.encounters[0] with{x=double.NaN}}},d with{encounters=new(){d.encounters[0] with{yaw_s16=32768}}}}){bool rejected=false;try{Config.Encode(bad);}catch{rejected=true;}Check("invalid-schema-id-numeric-yaw",rejected);}
  bool outside=false;try{g.Place(0,0);}catch{outside=true;}Check("outside-rejected",outside);
  using(var ui=new Editor()){Check("real-WinForms-created",ui.Text.Contains("DEVF"));ui.LoadForTest(d,g);Check("real-controls-and-model",ui.ModelCount==d.encounters.Count);ui.ExerciseControlsForTest(d,g);Check("actual-add-enable-yaw-drag-paint-controls",ui.ModelCount==2);}
  File.WriteAllText(Path.Combine(output,"result.json"),JsonSerializer.Serialize(new{status="CP08_APP_LAB_PASS",cases=passed.Count,case_names=passed,region="F_SP121/3/6 subset",geometry_sha256=Geometry.Hash,triangles=g.Triangles.Count,static_candidates=candidates,ground_north=g.Ground(-1250,61950),game_executed=false,interactive_human_UI=false},Config.Options));
 }
}
