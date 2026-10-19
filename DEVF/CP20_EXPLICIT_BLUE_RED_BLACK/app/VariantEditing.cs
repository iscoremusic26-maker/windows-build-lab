using System.Diagnostics;
using System.IO.Compression;
using System.Text.Json;
namespace DevfMap;
internal static class VariantEditing {
 internal static MultiEncounter Change(MultiEncounter e,string variant,int? hp){
  MultiConfig.C(variant is "blue" or "red" or "black","EXPLICIT_VARIANT_REQUIRED");
  MultiConfig.C(variant=="black"?hp is >=1 and <=999:hp==null,"HP_ONLY_BLACK");
  return e with{bokoblin_variant=variant,enemy=variant=="blue"?"E_OC":"E_OC2",hp=hp,enabled=false,qa_confirmed=false,layer_state="QA_CANDIDATE"};
 }
 internal static void Backup(string path,byte[] bytes){
  MultiConfig.Decode(bytes);
  using(var f=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None)){f.Write(bytes);f.Flush(true);}
  MultiConfig.C(File.ReadAllBytes(path).SequenceEqual(bytes),"CONVERSION_BACKUP_VERIFY");
 }
 internal static void RequireGameClosed(){
  foreach(var p in Process.GetProcesses()){using(p){try{MultiConfig.C(!p.ProcessName.Contains("dusklight",StringComparison.OrdinalIgnoreCase),"Cierre Dusklight antes de guardar o instalar");}catch(System.ComponentModel.Win32Exception){}}}
 }
 internal static void RequirePair(string mods){
  var found=new Dictionary<string,List<string>>{{"dev.bokoblin.expansion",new()},{"dev.devf.cp06_native_config_dual",new()}};
  foreach(var path in Directory.EnumerateFiles(mods,"*.dusk",SearchOption.AllDirectories)){
   using var z=ZipFile.OpenRead(path);var entries=z.Entries.Where(e=>e.FullName=="mod.json").ToArray();MultiConfig.C(entries.Length==1&&entries[0].Length<=16384,"MOD_MANIFEST_INVALID");
   using var stream=entries[0].Open();using var j=JsonDocument.Parse(stream);var root=j.RootElement;string? id=root.GetProperty("id").GetString();
   if(id!=null&&found.TryGetValue(id,out var list))list.Add(root.GetProperty("version").GetString()??"");
  }
  MultiConfig.C(found["dev.bokoblin.expansion"].Count==1&&found["dev.devf.cp06_native_config_dual"].Count==1,"CP20 requiere una sola Expansion y un solo Provider activos");
  MultiConfig.C(found["dev.bokoblin.expansion"][0]=="0.16.3"&&found["dev.devf.cp06_native_config_dual"][0]=="0.14.0","CP20 requiere Expansion 0.16.3 + Provider 0.14.0; no instalar JSON con Expansion 0.16.2");
 }
}
