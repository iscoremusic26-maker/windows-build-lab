using System.Text.Json;
namespace DevfMap;
// CP10.1 label resolver tests. Pure logic (no WinForms, no game data). Source data check: NameData.cs vs NAMES_PROVENANCE.md.
internal static class NameTests{
 public static List<string> Run(string output){
  var pass=new List<string>();void Ok(string n,bool v){if(!v)throw new Exception("NAME_TEST_FAILED "+n);pass.Add(n);}
  // 1 verified room name + technical code visible
  var grove=NameResolver.Resolve("F_SP117",1,"00");
  Ok("verified-room-name-with-code",grove.RoomName=="Sacred Grove"&&grove.Tier==NameTier.Room&&!grove.RoomInferred&&grove.Full=="Sacred Grove · Room 01 [F_SP117/R01_00.arc]"&&grove.Provenance.Contains("community map list"));
  Ok("verified-room-names-F_SP117-R02-R03",NameResolver.Resolve("F_SP117",2,"00").RoomName=="Temple of Time (Past)"&&NameResolver.Resolve("F_SP117",3,"00").RoomName=="Lost Woods");
  // 2 known stage, room without a specific name -> honest English fallback
  var field=NameResolver.Resolve("F_SP121",1,"00");
  Ok("known-stage-unnamed-room-fallback",field.StageName=="Hyrule Field"&&field.RoomName==null&&field.Short=="Room 01"&&field.Tier==NameTier.Stage&&field.Full=="Hyrule Field · Room 01 [F_SP121/R01_00.arc]");
  foreach(var r in new[]{1,3,4,5})Ok("F_SP121-R0"+r+"-no-invented-room-name",NameResolver.Resolve("F_SP121",r,"00").RoomName==null);
  var zora=NameResolver.Resolve("F_SP113",1,"00");
  Ok("F_SP113-R01-stage-name-only",zora.StageName=="Zora's Domain"&&zora.RoomName==null&&zora.Full=="Zora's Domain · Room 01 [F_SP113/R01_00.arc]");
  // 3 unknown stage and room: unambiguous technical identity, nothing invented
  var unknown=NameResolver.Resolve("F_SP999",42,"00");
  Ok("unknown-stage-room",unknown.Tier==NameTier.Fallback&&unknown.StageName==null&&unknown.RoomName==null&&unknown.Full=="Stage F_SP999 · Room 42 [F_SP999/R42_00.arc]"&&unknown.StageText=="Stage F_SP999");
  Ok("known-stage-unlisted-room",NameResolver.Resolve("F_SP113",7,"00").Full=="Zora's Domain · Room 07 [F_SP113/R07_00.arc]");
  // 4 same room number in other stages never shares an alias
  Ok("same-room-number-other-stage-no-alias",NameResolver.Resolve("F_SP999",1,"00").RoomName==null&&NameResolver.Resolve("F_SP121",1,"00").RoomName==null&&NameResolver.Resolve("F_SP113",1,"00").RoomName==null&&NameResolver.Resolve("F_SP117",1,"00").RoomName=="Sacred Grove"&&NameResolver.Resolve("R_SP01",1,"00").RoomName=="Sera's Sundries");
  Ok("same-room-number-distinct-full-labels",new[]{"F_SP117","R_SP01","F_SP121","F_SP113","F_SP999"}.Select(s=>NameResolver.Resolve(s,1,"00").Full).Distinct().Count()==5);
  // 5 variants of one room stay distinguishable
  var v0=NameResolver.Resolve("F_SP117",1,"00");var v1=NameResolver.Resolve("F_SP117",1,"01");
  Ok("variants-distinguishable",v0.Full!=v1.Full&&v0.Technical!=v1.Technical&&v1.Full.EndsWith("[F_SP117/R01_01.arc]")&&v0.Full.EndsWith("[F_SP117/R01_00.arc]"));
  // 6 STG resource has no room
  var stg=NameResolver.Resolve("F_SP121",null,"00");
  Ok("STG-resource-no-room",stg.Room==null&&stg.Short=="Stage Resource"&&stg.Full=="Hyrule Field · Stage Resource [F_SP121/STG_00.arc]"&&!stg.Full.Contains("Room"));
  Ok("STG-unknown-stage",NameResolver.Resolve("F_SP999",null,"00").Full=="Stage F_SP999 · Stage Resource [F_SP999/STG_00.arc]");
  // inferred names are flagged; never presented as verified
  var eld=NameResolver.Resolve("F_SP121",2,"00");
  Ok("inferred-room-flagged",eld.RoomInferred&&eld.Tier==NameTier.RoomInferred&&eld.Short=="Room 02 — Eldin Gorge south entrance (inferred)"&&eld.Full.Contains("(inferred)")&&eld.Provenance.StartsWith("Room name INFERRED"));
  // disputed sources -> fallback
  var dis=NameResolver.Resolve("F_SP118",0,"00");
  Ok("disputed-stage-fallback",dis.Dispute!=null&&dis.StageName==null&&dis.RoomName==null&&dis.Full=="Stage F_SP118 · Room 00 [F_SP118/R00_00.arc]"&&dis.StageText=="Stage F_SP118 (name disputed)"&&dis.Provenance.Contains("disagree"));
  // 7 duplicated display names still give unique full labels (name + technical id)
  Ok("duplicate-names-unique-full-labels",NameResolver.Resolve("R_SP109",3,"00").RoomName=="Malo Mart"&&NameResolver.Resolve("R_SP160",0,"00").RoomName=="Malo Mart"&&NameResolver.Resolve("R_SP109",3,"00").Full!=NameResolver.Resolve("R_SP160",0,"00").Full);
  var seen=new HashSet<string>();int total=0;
  foreach(var stage in NameData.Stages.Keys.Concat(NameData.Rooms.Keys.Select(k=>k.Stage)).Concat(NameData.Disputed.Keys).Distinct())
   foreach(var room in new int?[]{null,0,1,2,3,9,10})foreach(var variant in new[]{"00","01"}){total++;seen.Add(NameResolver.Resolve(stage,room,variant).Full);}
  Ok("all-full-labels-unique-per-identity",seen.Count==total);
  // data integrity: English ASCII only, no empty names, no alias attached to a disputed stage, rooms 0..255
  Ok("data-ascii-english-nonempty",NameData.Stages.Values.All(v=>v.Name.Length>0&&v.Name.All(c=>c<128))&&NameData.Rooms.Values.All(v=>v.Name.Length>0&&v.Name.All(c=>c<128)&&v.Source.Length>0));
  Ok("data-no-alias-for-disputed",NameData.Disputed.Keys.All(k=>!NameData.Stages.ContainsKey(k)&&!NameData.Rooms.Keys.Any(r=>r.Stage==k)));
  Ok("data-room-keys-valid",NameData.Rooms.Keys.All(k=>k.Room is >=0 and <=255&&k.Stage.Length>0));
  Ok("data-only-F_SP121-has-inferred",NameData.Rooms.Where(r=>r.Value.Inferred).All(r=>r.Key.Stage=="F_SP121")&&NameData.Rooms.Count(r=>r.Value.Inferred)==4);
  // labels never change identity: resolver is pure and independent of any editor state
  Ok("resolver-deterministic",NameResolver.Resolve("F_SP117",1,"00")==NameResolver.Resolve("F_SP117",1,"00"));
  Ok("null-safe",NameResolver.Resolve(null!,null,null!).Full.Contains("Stage Resource"));
  File.WriteAllText(Path.Combine(output,"names-result.json"),JsonSerializer.Serialize(new{status="CP10_1_NAME_LAB_PASS",cases=pass.Count,case_names=pass,stages_named=NameData.Stages.Count,room_entries=NameData.Rooms.Count,rooms_inferred=NameData.Rooms.Count(r=>r.Value.Inferred),stages_disputed=NameData.Disputed.Count,runtime="HUMAN_VISUAL_PENDING"}));
  return pass;
 }
}
