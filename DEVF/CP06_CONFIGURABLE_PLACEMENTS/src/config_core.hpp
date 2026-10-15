#pragma once
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace cp6 {
constexpr std::size_t kMaxBytes=16384;
struct Error {const char* code="NONE";};
struct Node {
 enum Type{Object,Array,String,Number,Bool} type=Object;
 std::string text;double number=0;bool boolean=false;
 std::vector<std::pair<std::string,Node>> object;std::vector<Node> array;
};
struct Parser {
 std::string_view s;std::size_t p=0;unsigned nodes=0;Error error;
 bool fail(const char* why){error.code=why;return false;}
 void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\t'||s[p]=='\r'||s[p]=='\n'))++p;}
 bool take(char c){ws();if(p<s.size()&&s[p]==c){++p;return true;}return false;}
 bool str(std::string& out){
  if(!take('"'))return fail("JSON_STRING");out.clear();
  while(p<s.size()&&s[p]!='"'){unsigned char c=s[p++];if(c<32||c>126||c=='\\')return fail("JSON_STRING_ENCODING");if(out.size()==64)return fail("JSON_STRING_LIMIT");out+=char(c);}
  if(p==s.size())return fail("JSON_STRING_UNTERMINATED");++p;return true;
 }
 bool value(Node& n,unsigned depth){
  if(depth>8||++nodes>512)return fail("JSON_COMPLEXITY");ws();if(p==s.size())return fail("JSON_EOF");
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
 bool parse(Node& root){if(s.empty()||s.size()>kMaxBytes)return fail("CONFIG_SIZE");if(!value(root,0))return false;ws();return p==s.size()||fail("JSON_TRAILING");}
};
inline const Node* get(const Node& n,std::string_view key){for(const auto& x:n.object)if(x.first==key)return &x.second;return nullptr;}
inline bool keys(const Node& n,std::initializer_list<std::string_view> expected){
 if(n.type!=Node::Object||n.object.size()!=expected.size())return false;
 for(auto k:expected)if(!get(n,k))return false;return true;
}
inline bool id(const Node* n){if(!n||n->type!=Node::String||n->text.empty()||n->text.size()>32)return false;for(char c:n->text)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;return true;}
inline bool num(const Node* n,double min,double max){return n&&n->type==Node::Number&&std::isfinite(n->number)&&n->number>=min&&n->number<=max;}
inline bool integer(const Node* n,double min,double max){return num(n,min,max)&&std::floor(n->number)==n->number;}
inline bool text(const Node* n,std::string_view expected){return n&&n->type==Node::String&&n->text==expected;}
struct Variant {std::string id;bool black=false,guard=false;std::int16_t hp=220;};
struct Placement {
 std::string id,variant;bool enabled=false;float x=0,y=0,z=0;std::int16_t yaw=0;
};
struct Config {std::array<Variant,2> variants{};std::array<Placement,2> placements{};unsigned variant_count=0,count=0;int selected=-1;};
inline const Variant* variant(const Config& c,const std::string& id){for(unsigned i=0;i<c.variant_count;++i)if(c.variants[i].id==id)return &c.variants[i];return nullptr;}
inline bool decode(std::string_view raw,Config& out,Error& error){
 out=Config{};try{
  Parser parser{raw};Node root;if(!parser.parse(root)){error=parser.error;return false;}
  auto bad=[&](const char* why){error.code=why;return false;};
  if(!keys(root,{"schema_version","variants","placements"}))return bad("SCHEMA_ROOT_KEYS");
  if(!integer(get(root,"schema_version"),1,1))return bad("SCHEMA_VERSION");
  const auto* vs=get(root,"variants");const auto* ps=get(root,"placements");
  if(vs->type!=Node::Array||vs->array.empty()||vs->array.size()>2)return bad("CATALOG_LIMIT");
  if(ps->type!=Node::Array||ps->array.size()>2)return bad("PLACEMENT_LIMIT");
  Config draft;
  for(const auto& v:vs->array){
   if(!keys(v,{"variant_id","enemy","native_profile","hp","appearance","ai"})||!id(get(v,"variant_id")))return bad("VARIANT_FIELDS");
   if(!text(get(v,"enemy"),"E_OC2")||!integer(get(v,"native_profile"),510,510))return bad("UNSUPPORTED_NATIVE_PROFILE");
   Variant item;item.id=get(v,"variant_id")->text;
   if(variant(draft,item.id))return bad("DUPLICATE_VARIANT_ID");
   if(integer(get(v,"hp"),150,150)&&text(get(v,"appearance"),"charcoal")&&text(get(v,"ai"),"bounded_guard")){item.hp=150;item.black=item.guard=true;}
   else if(integer(get(v,"hp"),220,220)&&text(get(v,"appearance"),"native")&&text(get(v,"ai"),"native")){}
   else return bad("UNSUPPORTED_VARIANT_COMBINATION");
   draft.variants[draft.variant_count++]=std::move(item);
  }
  for(const auto& p:ps->array){
   if(!keys(p,{"placement_id","variant_id","enabled","stage","room","layer","x","y","z","yaw_s16","parameters"})||!id(get(p,"placement_id"))||!id(get(p,"variant_id"))||get(p,"enabled")->type!=Node::Bool)return bad("PLACEMENT_FIELDS");
   Placement item;item.id=get(p,"placement_id")->text;item.variant=get(p,"variant_id")->text;item.enabled=get(p,"enabled")->boolean;
   for(unsigned i=0;i<draft.count;++i)if(draft.placements[i].id==item.id)return bad("DUPLICATE_PLACEMENT_ID");
   if(!variant(draft,item.variant))return bad("UNKNOWN_VARIANT_ID");
   if(!text(get(p,"stage"),"F_SP121")||!integer(get(p,"room"),3,3)||!integer(get(p,"layer"),6,6))return bad("SCENE_NOT_VERIFIED");
   if(!num(get(p,"x"),-1000000,1000000)||!num(get(p,"y"),-100000,100000)||!num(get(p,"z"),-1000000,1000000))return bad("COORDINATE_RANGE");
   if(!integer(get(p,"yaw_s16"),-32768,32767)||!integer(get(p,"parameters"),4294902016.,4294902016.))return bad("NATIVE_RECORD_CONTRACT");
   item.x=static_cast<float>(get(p,"x")->number);item.y=static_cast<float>(get(p,"y")->number);item.z=static_cast<float>(get(p,"z")->number);item.yaw=static_cast<std::int16_t>(get(p,"yaw_s16")->number);
   // Only the human-observed site is admitted. Do not fabricate a second safe coordinate.
   if(item.x!=-1250.f||item.y!=-6304.00830078125f||item.z!=60750.f)return bad("SITE_NOT_VERIFIED");
   if(item.enabled){if(draft.selected!=-1)return bad("MULTI_ACTIVE_NOT_SUPPORTED");draft.selected=int(draft.count);}
   draft.placements[draft.count++]=std::move(item);
  }
  out=std::move(draft);error.code="NONE";return true;
 }catch(...){out=Config{};error.code="CONFIG_ALLOCATION_OR_PARSE_FAILURE";return false;}
}
inline Config config;
inline const Placement* active(){return config.selected<0?nullptr:&config.placements[config.selected];}
inline const Variant* active_variant(){const auto* p=active();return p?variant(config,p->variant):nullptr;}
inline bool black(){auto* v=active_variant();return v&&v->black;}
inline bool guard(){auto* v=active_variant();return v&&v->guard;}
inline std::int16_t hp(){auto* v=active_variant();return v?v->hp:220;}
}
