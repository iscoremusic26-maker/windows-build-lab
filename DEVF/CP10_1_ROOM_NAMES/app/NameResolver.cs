namespace DevfMap;
// CP10.1 editor-only display labels (English). Identity is ALWAYS the technical id (stage/room/variant); labels are never stored,
// never used as keys, never written to JSON and never gate QA or export. Data: NameData.cs (see NAMES_PROVENANCE.md).
public enum NameTier { Fallback, Stage, Room, RoomInferred }
public sealed record NameLabel(string Stage,int? Room,string Variant,string Technical,string? StageName,string? RoomName,bool RoomInferred,string? Dispute,NameTier Tier,string Source){
 public string StageText=>StageName!=null?$"{StageName} — {Stage}":Dispute!=null?$"Stage {Stage} (name disputed)":$"Stage {Stage}";
 public string Place=>RoomName!=null?(RoomInferred?$"{StageName??"Stage "+Stage} · {RoomName} (inferred)":RoomName):(StageName??$"Stage {Stage}");
 public string Short=>Room.HasValue?$"Room {Room:00}"+(RoomName!=null?$" — {RoomName}"+(RoomInferred?" (inferred)":""):""):"Stage Resource";
 public string Full=>Room.HasValue?$"{Place} · Room {Room:00} [{Technical}]":$"{Place} · Stage Resource [{Technical}]";
 public string Provenance=>Tier switch{
  NameTier.Room=>"Room name documented by the community map list ("+Source+").",
  NameTier.RoomInferred=>"Room name INFERRED from a source comment, not verified in-game: "+Source,
  NameTier.Stage=>"Stage name documented ("+Source+"); no specific room name known for this room.",
  _=>Dispute!=null?"No name shown. Sources disagree: "+Dispute:"No verified name for this stage/room; showing technical identity only."};
}
public static class NameResolver {
 static string Technical(string stage,int? room,string variant)=>room.HasValue?$"{stage}/R{room.Value:00}_{variant}.arc":$"{stage}/STG_{variant}.arc";
 public static NameLabel Resolve(string stage,int? room,string variant){
  stage??="";variant??="";
  NameData.Disputed.TryGetValue(stage,out var dispute);
  string? stageName=null,source="";
  if(dispute==null&&NameData.Stages.TryGetValue(stage,out var s)){stageName=s.Name;source=s.Source;}
  string? roomName=null;bool inferred=false;
  if(dispute==null&&room.HasValue&&NameData.Rooms.TryGetValue((stage,room.Value),out var r)){roomName=r.Name;inferred=r.Inferred;source=r.Source;}
  var tier=roomName!=null?(inferred?NameTier.RoomInferred:NameTier.Room):stageName!=null?NameTier.Stage:NameTier.Fallback;
  return new NameLabel(stage,room,variant,Technical(stage,room,variant),stageName,roomName,inferred,dispute,tier,source);
 }
 public static string StageText(string stage)=>Resolve(stage,null,"00").StageText;
 public static string StageTip(string stage){var l=Resolve(stage,null,"00");return l.StageText+"\n"+(l.StageName!=null?"Stage name documented ("+l.Source+")":l.Provenance);}
}
