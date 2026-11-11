#pragma once
#include "config_core.hpp"
#include <cstdio>
#include <cstring>
namespace encounter {
using native_config::Node;using native_config::Error;
constexpr unsigned MaxEncounters=16,MaxQuantity=4;
struct Parser {
 std::string_view s;std::size_t p=0;unsigned nodes=0;Error error;
 bool fail(const char* why){error.code=why;return false;}
 void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\t'||s[p]=='\r'||s[p]=='\n'))++p;}
 bool take(char c){ws();if(p<s.size()&&s[p]==c){++p;return true;}return false;}
 static void utf(std::string& out,unsigned c){if(c<128)out+=char(c);else if(c<2048){out+=char(192|(c>>6));out+=char(128|(c&63));}else if(c<65536){out+=char(224|(c>>12));out+=char(128|((c>>6)&63));out+=char(128|(c&63));}else{out+=char(240|(c>>18));out+=char(128|((c>>12)&63));out+=char(128|((c>>6)&63));out+=char(128|(c&63));}}
 bool hex(unsigned& v){v=0;for(unsigned i=0;i<4;++i){if(p==s.size())return false;char c=s[p++];unsigned n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:99;if(n>15)return false;v=v*16+n;}return true;}
 bool str(std::string& out){
  if(!take('"'))return fail("JSON_STRING");out.clear();
  while(p<s.size()&&s[p]!='"'){
   unsigned char c=s[p++];if(c<32)return fail("JSON_CONTROL");
   if(c=='\\'){if(p==s.size())return fail("JSON_ESCAPE");char e=s[p++];switch(e){
    case '"':out+='"';break;case '\\':out+='\\';break;case '/':out+='/';break;
    case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;
    case 'u':{unsigned u=0;if(!hex(u))return fail("JSON_UNICODE");if(u>=0xD800&&u<=0xDBFF){if(s.substr(p,2)!="\\u")return fail("JSON_SURROGATE");p+=2;unsigned low=0;if(!hex(low)||low<0xDC00||low>0xDFFF)return fail("JSON_SURROGATE");u=0x10000+((u-0xD800)<<10)+low-0xDC00;}else if(u>=0xDC00&&u<=0xDFFF)return fail("JSON_SURROGATE");utf(out,u);break;}
    default:return fail("JSON_ESCAPE");
   }}else out+=char(c);
   if(out.size()>192)return fail("JSON_STRING_LIMIT");
  }
  if(p==s.size())return fail("JSON_STRING_UNTERMINATED");++p;return true;
 }
 bool value(Node& n,unsigned depth){
  if(depth>8||++nodes>4096)return fail("JSON_COMPLEXITY");ws();if(p==s.size())return fail("JSON_EOF");
  if(s[p]=='{'){++p;n.type=Node::Object;if(take('}'))return true;do{
   std::string key;if(!str(key)||!take(':'))return fail("JSON_OBJECT");
   for(const auto& x:n.object)if(x.first==key)return fail("JSON_DUPLICATE_KEY");
   Node child;if(!value(child,depth+1))return false;n.object.emplace_back(key,std::move(child));
  }while(take(','));return take('}')||fail("JSON_OBJECT_END");}
  if(s[p]=='['){++p;n.type=Node::Array;if(take(']'))return true;do{Node child;if(!value(child,depth+1))return false;n.array.push_back(std::move(child));}while(take(','));return take(']')||fail("JSON_ARRAY_END");}
  if(s[p]=='"'){n.type=Node::String;return str(n.text);}
  for(auto token:{std::string_view("true"),std::string_view("false")})if(s.substr(p,token.size())==token){p+=token.size();n.type=Node::Bool;n.boolean=token=="true";return true;}
  const auto start=p;if(s[p]=='-')++p;if(p==s.size())return fail("JSON_NUMBER");
  if(s[p]=='0')++p;else {if(s[p]<'1'||s[p]>'9')return fail("JSON_NUMBER");while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;}
  if(p<s.size()&&s[p]=='.'){++p;auto d=p;while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;if(d==p)return fail("JSON_NUMBER");}
  if(p<s.size()&&(s[p]=='e'||s[p]=='E')){++p;if(p<s.size()&&(s[p]=='+'||s[p]=='-'))++p;auto d=p;while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;if(d==p)return fail("JSON_NUMBER");}
  n.type=Node::Number;auto result=std::from_chars(s.data()+start,s.data()+p,n.number);
  return (result.ec==std::errc{}&&result.ptr==s.data()+p&&std::isfinite(n.number))||fail("JSON_NUMBER_RANGE");
 }
 bool parse(Node& root){if(s.empty()||s.size()>16384)return fail("CONFIG_SIZE");if(!value(root,0))return false;ws();return p==s.size()||fail("JSON_TRAILING");}
};
using native_config::get;using native_config::keys;using native_config::integer;using native_config::num;using native_config::text;
inline bool utf8(std::string_view s){
 for(size_t i=0;i<s.size();){unsigned char c=s[i++];if(c<32||c==127)return false;if(c<128)continue;
 unsigned n=c>=0xC2&&c<=0xDF?1:c>=0xE0&&c<=0xEF?2:c>=0xF0&&c<=0xF4?3:9;if(n==9||i+n>s.size())return false;
 unsigned cp=c&((1u<<(6-n))-1);for(unsigned j=0;j<n;++j){unsigned char t=s[i++];if((t&0xC0)!=0x80)return false;cp=(cp<<6)|(t&63);}
 if(cp<(n==1?128u:n==2?2048u:65536u)||cp>0x10FFFF||(cp>=0xD800&&cp<=0xDFFF))return false;
 }return true;
}
inline bool stage_ok(std::string_view s){if(s.empty()||s.size()>7)return false;for(char c:s)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false;return true;}
inline bool id_ok(std::string_view s){if(s.empty()||s.size()>32)return false;for(char c:s)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;return true;}
struct Sample {
 std::string stage;int room=-1,layer=-1,yaw=0;double x=0,y=0,z=0;uint64_t tick=0;
 std::string origin="NONE";bool valid=false;
};
inline bool sample_ok(const Sample& s){return s.valid&&s.origin=="link_readonly_v203"&&stage_ok(s.stage)&&s.room>=0&&s.room<64&&s.layer>=0&&s.layer<16&&s.yaw>=-32768&&s.yaw<=32767&&s.tick>0&&s.tick<9007199254740992ULL&&std::isfinite(s.x)&&std::isfinite(s.y)&&std::isfinite(s.z)&&std::abs(s.x)<=1000000&&std::abs(s.y)<=1000000&&std::abs(s.z)<=1000000;}
struct CaptureCheck {bool same_scene=true,transition=false,event=false,member_ready=true;uint64_t requested_tick=0,read_tick=0;};
inline bool capture_gate(const Sample& s,const CaptureCheck& g){return g.same_scene&&!g.transition&&!g.event&&g.member_ready&&g.requested_tick>0&&g.read_tick>=g.requested_tick&&g.read_tick-g.requested_tick<=2&&s.tick==g.read_tick&&sample_ok(s);}
struct Entry {std::string id,name;Sample sample;unsigned quantity=1,spacing=150,respawn=0;int yaw=0;};
using Collection=std::vector<Entry>;
inline bool entry_ok(const Entry& e){return id_ok(e.id)&&!e.name.empty()&&e.name.size()<=96&&utf8(e.name)&&sample_ok(e.sample)&&e.quantity>=1&&e.quantity<=MaxQuantity&&e.spacing<=1000&&e.respawn<=1&&e.yaw>=-32768&&e.yaw<=32767;}
inline bool collection_ok(const Collection& c){if(c.size()>MaxEncounters)return false;for(size_t i=0;i<c.size();++i){if(!entry_ok(c[i]))return false;for(size_t j=0;j<i;++j)if(c[j].id==c[i].id||c[j].name==c[i].name)return false;}return true;}
inline std::string quote(std::string_view s){std::string o="\"";for(unsigned char c:s){if(c=='"'||c=='\\'){o+='\\';o+=char(c);}else if(c<32){char b[7];std::snprintf(b,sizeof(b),"\\u%04x",c);o+=b;}else o+=char(c);}return o+'"';}
inline std::string number(double n){char b[64];auto r=std::to_chars(b,b+sizeof(b),n,std::chars_format::general,17);if(r.ec!=std::errc{})return "INVALID";return {b,r.ptr};}
inline std::string serialize(const Collection& c,bool exported=false){
 std::string o="{\"document_type\":"+quote(exported?"devf_encounter_export":"devf_encounter_collection")+",\"schema_version\":1,\"producer\":\"dev.devf.cp06_native_config_dual@0.7.2\",\"runtime_applied\":false,\"encounters\":[";
 for(size_t i=0;i<c.size();++i){const auto& e=c[i];const auto& s=e.sample;if(i)o+=',';
 o+="{\"encounter_id\":"+quote(e.id)+",\"name\":"+quote(e.name)+",\"enemy\":\"E_OC2\",\"variant_id\":\"native_elite\",\"native_profile\":510,\"stage\":"+quote(s.stage)+",\"room\":"+std::to_string(s.room)+",\"layer\":"+std::to_string(s.layer)+",\"x\":"+number(s.x)+",\"y\":"+number(s.y)+",\"z\":"+number(s.z)+",\"yaw_s16\":"+std::to_string(e.yaw)+",\"capture_yaw_s16\":"+std::to_string(s.yaw)+",\"quantity\":"+std::to_string(e.quantity)+",\"spacing_units\":"+std::to_string(e.spacing)+",\"respawn_policy\":"+quote(e.respawn?"no_respawn_during_session":"on_zone_reentry")+",\"respawn_applied\":false,\"capture_origin\":"+quote(s.origin)+",\"capture_tick\":"+std::to_string(s.tick)+",\"capture_validity\":\"READ_ONLY_SNAPSHOT_NOT_TERRAIN_VERIFIED\",\"terrain_verified\":false}";
 }return o+"]}\n";
}
inline bool decode(std::string_view raw,Collection& out,bool exported,Error& error){
 out.clear();try{Node root;Parser p{raw};if(!p.parse(root)){error=p.error;return false;}auto bad=[&](const char* s){error.code=s;return false;};
 if(!keys(root,{"document_type","schema_version","producer","runtime_applied","encounters"})||!text(get(root,"document_type"),exported?"devf_encounter_export":"devf_encounter_collection")||!integer(get(root,"schema_version"),1,1)||!text(get(root,"producer"),"dev.devf.cp06_native_config_dual@0.7.2")||get(root,"runtime_applied")->type!=Node::Bool||get(root,"runtime_applied")->boolean)return bad("ENCOUNTER_SCHEMA");
 auto* a=get(root,"encounters");if(a->type!=Node::Array||a->array.size()>MaxEncounters)return bad("ENCOUNTER_LIMIT");Collection draft;
 for(const auto& n:a->array){
  if(!keys(n,{"encounter_id","name","enemy","variant_id","native_profile","stage","room","layer","x","y","z","yaw_s16","capture_yaw_s16","quantity","spacing_units","respawn_policy","respawn_applied","capture_origin","capture_tick","capture_validity","terrain_verified"}))return bad("ENCOUNTER_FIELDS");
  for(auto k:{"encounter_id","name","stage","capture_origin"})if(get(n,k)->type!=Node::String)return bad("ENCOUNTER_TEXT");
  if(!text(get(n,"enemy"),"E_OC2")||!text(get(n,"variant_id"),"native_elite")||!integer(get(n,"native_profile"),510,510))return bad("NATIVE_CATALOG_ONLY");
  for(auto k:{"runtime_applied"}){(void)k;}
  if(!text(get(n,"capture_validity"),"READ_ONLY_SNAPSHOT_NOT_TERRAIN_VERIFIED")||get(n,"terrain_verified")->type!=Node::Bool||get(n,"terrain_verified")->boolean||get(n,"respawn_applied")->type!=Node::Bool||get(n,"respawn_applied")->boolean)return bad("UNSUPPORTED_RUNTIME_CLAIM");
  if(!integer(get(n,"room"),0,63)||!integer(get(n,"layer"),0,15)||!integer(get(n,"yaw_s16"),-32768,32767)||!integer(get(n,"capture_yaw_s16"),-32768,32767)||!integer(get(n,"quantity"),1,MaxQuantity)||!integer(get(n,"spacing_units"),0,1000)||!integer(get(n,"capture_tick"),1,9007199254740991.)||!num(get(n,"x"),-1000000,1000000)||!num(get(n,"y"),-1000000,1000000)||!num(get(n,"z"),-1000000,1000000))return bad("ENCOUNTER_RANGES");
  if(!text(get(n,"respawn_policy"),"on_zone_reentry")&&!text(get(n,"respawn_policy"),"no_respawn_during_session"))return bad("RESPAWN_METADATA_ONLY");
  Entry e;e.id=get(n,"encounter_id")->text;e.name=get(n,"name")->text;e.sample={get(n,"stage")->text,int(get(n,"room")->number),int(get(n,"layer")->number),int(get(n,"capture_yaw_s16")->number),get(n,"x")->number,get(n,"y")->number,get(n,"z")->number,uint64_t(get(n,"capture_tick")->number),get(n,"capture_origin")->text,true};
  e.yaw=int(get(n,"yaw_s16")->number);e.quantity=unsigned(get(n,"quantity")->number);e.spacing=unsigned(get(n,"spacing_units")->number);e.respawn=text(get(n,"respawn_policy"),"no_respawn_during_session")?1:0;draft.push_back(std::move(e));
 }if(!collection_ok(draft))return bad("INVALID_OR_DUPLICATE_ENCOUNTER");out=std::move(draft);error.code="NONE";return true;
 }catch(...){error.code="ENCOUNTER_PARSE_ALLOCATION";out.clear();return false;}
}
struct Model {
 Collection saved,working;Entry draft;int selected=-1;bool alive=false,busy=false,delete_confirmed=false;const char* message="NOT_STARTED";
 void fresh(){selected=-1;delete_confirmed=false;draft=Entry{};for(unsigned i=1;i<=MaxEncounters+1;++i){draft.id="encounter-"+std::to_string(i);bool used=false;for(const auto& e:working)if(e.id==draft.id)used=true;if(!used){draft.name="Encuentro "+std::to_string(i);break;}}message="NEW_DRAFT_CAPTURE_REQUIRED";}
 bool capture(const Sample& s){if(!alive||busy||!sample_ok(s)){message="CAPTURE_REJECTED";return false;}auto next=draft;next.sample=s;next.yaw=s.yaw;draft=std::move(next);message="LINK_SNAPSHOT_COPIED_NO_SPAWN";return true;}
 bool apply(){if(!alive||busy||!entry_ok(draft)){message="INVALID_DRAFT";return false;}auto next=working;if(selected<0){if(next.size()==MaxEncounters){message="ENCOUNTER_LIMIT";return false;}next.push_back(draft);}else if(size_t(selected)<next.size())next[selected]=draft;else return false;if(!collection_ok(next)){message="DUPLICATE_ID_OR_NAME";return false;}working=std::move(next);selected=int(working.size())-1;for(size_t i=0;i<working.size();++i)if(working[i].id==draft.id)selected=int(i);message="COLLECTION_DRAFT_UPDATED_NO_SPAWN";return true;}
 void select(unsigned i){if(!alive||busy||i>=working.size())return;auto next=working[i];selected=int(i);draft=std::move(next);delete_confirmed=false;message="SELECTED_DRAFT";}
 void discard_draft(){if(selected>=0&&size_t(selected)<working.size())draft=working[selected];else fresh();delete_confirmed=false;message="DRAFT_DISCARDED";}
 bool erase(){if(!alive||busy||!delete_confirmed||selected<0||size_t(selected)>=working.size()){message="DELETE_CONFIRM_REQUIRED";return false;}working.erase(working.begin()+selected);fresh();message="DELETED_FROM_DRAFT_COLLECTION";return true;}
 void discard_collection(){working=saved;fresh();message="COLLECTION_DISCARDED_TO_SAVED";}
 bool dirty()const{return serialize(working)!=serialize(saved);}
};
bool read_native(uint64_t tick,Sample& out,const char*& reason) noexcept;
}
