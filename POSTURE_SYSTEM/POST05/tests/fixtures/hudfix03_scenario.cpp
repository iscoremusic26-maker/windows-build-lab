static unsigned checks=0;static void ck(bool b,const char* m){++checks;if(!b){std::fprintf(stderr,"UI03_FAIL %s\n",m);std::exit(1);}}
static std::uint64_t tag(const char* s){std::uint64_t a=0;while(*s)a=(a<<8)|static_cast<unsigned char>(*s++);return a;}
static void link(J2DPane& p,J2DPane& c){p.mPaneTree.children.push_back({&c});c.mPaneTree.parent=&p.mPaneTree;}
static bool hooks=true;static std::vector<int> draws;static unsigned root_disabled=0,fx_updates=0;
// Independent native traversal follows J2DPane::draw: false draw argument
// propagates to children while matrices/caches still update. No planner calls.
static void native(J2DPane& p,const Mtx* parent,const J2DGrafContext* graph,float width,bool enabled=true){
 J2DPane* self=&p;float x=0,y=0;bool ortho=true;void* args[]={&self,&x,&y,&graph,&ortho,&enabled};const bool saved=enabled;
 if(hooks)on_pane_pre_fix02(nullptr,args,nullptr,nullptr);
 if(p.getTypeID()==8&&!enabled)++root_disabled;
 if(!parent){const float sx=p.mScaleX,tx=p.mTranslateX;p.mScaleX*=width/608;p.mTranslateX=(tx-304)*width/608+304;p.makeMatrix(p.mTranslateX+x,p.mTranslateY+y);p.mScaleX=sx;p.mTranslateX=tx;}
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float v=parent?(c==3?(*parent)[r][3]:0):p.mPositionMtx[r][c];if(parent)for(unsigned k=0;k<3;++k)v+=(*parent)[r][k]*p.mPositionMtx[k][c];p.mGlobalMtx[r][c]=v;}
 enabled=enabled&&p.mVisible;
 if(enabled&&p.mAlpha&&p.draw_id>=0)draws.push_back(p.draw_id);
 for(auto c:p.mPaneTree.children)native(*c.p,&p.mGlobalMtx,graph,width,enabled);
 if(hooks)on_pane_post_fix02(nullptr,args,nullptr,nullptr);
 ck(enabled==saved||!p.mVisible,"native draw argument lease restored");
}
static void screen(J2DScreen& s,J2DOrthoGraph& graph){const J2DGrafContext* port=&graph;J2DScreen* p=&s;float x=0,y=0;void* a[]={&p,&x,&y,&port};if(hooks)on_screen_pre_fix02(nullptr,a,nullptr,nullptr);native(s,nullptr,port,graph.ortho.f.x-graph.ortho.i.x);if(hooks)on_screen_post_fix02(nullptr,a,nullptr,nullptr);}
struct Scene{
 J2DScreen root,pikari,general;J2DPane general_ui[10];J2DPane shared,protected_ui[10],text[2],letters[2][5],icon[2],glyph[2][2],effect,flash,foreign_fx;
 CPaneMgr text_manager[2],icon_manager[2],fx_manager;dMeterButton_c owner;dMeter2Draw_c md;dMeter2_c hud;
 Scene(){
  root.place(0,0,608,448);shared.place(0,0,608,448);link(root,shared);
  for(unsigned i=0;i<10;++i){protected_ui[i].place(20+float(i%3)*180,20+float(i%4)*95,50,20,i%2?19:18,int(i));link(shared,protected_ui[i]);}
  // Non-prompt picture crossing the band is protected even with exact overlap.
  protected_ui[9].place(300,385,20,20,18,9);
  for(unsigned s=0;s<2;++s){text[s].place(s?20:240,s?315:375,100,24);text[s].mInfoTag=tag(s?"text2_n":"text_n");link(shared,text[s]);
   icon[s].place(s?125:350,s?315:375,24,24);icon[s].mInfoTag=tag(s?"bottl_n":"bbtn_n");link(shared,icon[s]);
   for(unsigned j=0;j<5;++j){letters[s][j].place(float(j)*.2f,float(j)*.2f,100,20,19,100+int(s)*20+int(j));link(text[s],letters[s][j]);}
   for(unsigned j=0;j<2;++j){glyph[s][j].place(float(j),float(j),22,22,18,110+int(s)*20+int(j));link(icon[s],glyph[s][j]);}
   text_manager[s].pane=&text[s];icon_manager[s].pane=&icon[s];owner.mpText[s]=&text_manager[s];}
  owner.mpButtonScreen=&root;owner.mpButtonB=&icon_manager[0];owner.mpButtonBin=&icon_manager[1];
  pikari.place(0,0,608,448);effect.place(350,375,25,25);effect.mInfoTag=tag("pika_n");flash.place(0,0,25,25,18,300);foreign_fx.place(510,10,20,20,18,390);link(pikari,effect);link(effect,flash);link(pikari,foreign_fx);fx_manager.pane=&effect;
  general.place(0,0,608,448);for(unsigned i=0;i<10;++i){general_ui[i].place(20+float(i)*40,i==9?385:30,40,20,i%2?19:18,40+int(i));link(general,general_ui[i]);}
  md.mpScreen=&general;md.mpPikariScreen=&pikari;md.mpPikariParent=&fx_manager;hud.mpEmpButton=&owner;hud.mpMeterDraw=&md;meter=&hud;
 }
};
static void begin_owner(dMeterButton_c* p){void* a[]={&p};on_prompt_pre_fix02(nullptr,a,nullptr,nullptr);}
// Independent geometry oracle uses matrices actually produced by the native
// traversal before hooks, not the production collector or its slot decision.
struct NativeBounds{float l=1e9f,t=1e9f,r=-1e9f,b=-1e9f;};
static void include(NativeBounds& b,const J2DPane& p){if(!p.mVisible||!p.mAlpha)return;for(unsigned k=0;k<4;++k){const float x=k&1?p.mBounds.f.x:p.mBounds.i.x,y=k&2?p.mBounds.f.y:p.mBounds.i.y;const float wx=p.mGlobalMtx[0][0]*x+p.mGlobalMtx[0][1]*y+p.mGlobalMtx[0][3],wy=p.mGlobalMtx[1][0]*x+p.mGlobalMtx[1][1]*y+p.mGlobalMtx[1][3];b.l=std::min(b.l,wx);b.t=std::min(b.t,wy);b.r=std::max(b.r,wx);b.b=std::max(b.b,wy);}}
static bool native_conflict(Scene& s,unsigned slot){NativeBounds b;for(auto& p:s.letters[slot])include(b,p);if(s.icon[slot].mVisible)for(auto& p:s.glyph[slot])include(b,p);const auto band=hf.present.band;return b.l<band.x1&&b.r>band.x0&&b.t<band.y1&&b.b>band.y0;}
static bool has(int id){return std::find(draws.begin(),draws.end(),id)!=draws.end();}
static void direct_picture(J2DPane* p,int id,float x=300,float y=385){float w=20,h=20;void* a[]={&p,&x,&y,&w,&h};if(!hooks||on_picture_pre_fix02(nullptr,a,nullptr,nullptr)==HOOK_CONTINUE)draws.push_back(id);}
static void direct_string(int id){JUTFont f;JUTFont* fp=&f;const char* text="Shield attack";u32 n=13;bool first=true;float x=260,y=391,w=8,h=12;void* a[]={&fp,&x,&y,&w,&h,&text,&n,&first};if(hooks)on_string_pre_fix02(nullptr,a,nullptr,nullptr);if(f.mColor1.a)draws.push_back(id);const float native_advance=n*w;if(hooks)on_string_post_fix02(nullptr,a,nullptr,nullptr);ck(f.mColor1.a==255&&f.mColor2.a==255&&f.mColor3.a==255&&f.mColor4.a==255&&native_advance==104,"foreign font colours/advance exact");}
static void outfont(J2DPane* text,int id){void* font=nullptr;void* a[]={&font,&text};if(!hooks||on_outfont_pre_fix02(nullptr,a,nullptr,nullptr)==HOOK_CONTINUE)draws.push_back(id);}
static void pikari(Scene& s,J2DOrthoGraph& graph,float* frame){dMeter2Draw_c* md=&s.md;float x=350,y=375;void* a[]={&md,&x,&y,&frame};
#ifndef HUD03_BASE
 if(hooks)on_pikari_pre_fix03(nullptr,a,nullptr,nullptr);
#endif
 *frame+=.25f;++fx_updates;screen(s.pikari,graph); // ALWAYS original visual update
#ifndef HUD03_BASE
 if(hooks)on_pikari_post_fix03(nullptr,a,nullptr,nullptr);
#endif
}
static constexpr const char* button_tags[22]={"abtn_n","bbtn_n","rbtn_n","zbtn_n","asbtn_n","cbtn_n","sbtn_n","xbtn_n","ybtn_n",nullptr,nullptr,nullptr,nullptr,"as_b_n",nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,"bottl_n"};
static void code(Scene& s,unsigned c){CPaneMgr** p[22]={&s.owner.mpButtonA,&s.owner.mpButtonB,&s.owner.mpButtonR,&s.owner.mpButtonZ,&s.owner.mpButton3D,&s.owner.mpButtonC,&s.owner.mpButtonS,&s.owner.mpButtonX,&s.owner.mpButtonY,&s.owner.mpButtonNun,&s.owner.mpButtonRemo,&s.owner.mpButtonRemo2,&s.owner.mpButtonAR,&s.owner.mpButton3DB,&s.owner.mpButtonNURE,&s.owner.mpButtonReel,&s.owner.mpButtonReel2,&s.owner.mpButtonAB,&s.owner.mpButtonTate,&s.owner.mpButtonNunZ,&s.owner.mpButtonNunC,&s.owner.mpButtonBin};
 for(unsigned i=0;i<22;++i)*p[i]=nullptr;
 // PC constructor leaves Wii-only image fields NULL. The text slot still has
 // native provenance; no invented tags or live Wii widgets are asserted.
 if(button_tags[c]){*p[c]=&s.icon_manager[0];s.icon[0].mInfoTag=tag(button_tags[c]);}else{s.icon[0].mVisible=false;}
 s.owner.field_0x4be[0]=static_cast<u8>(c);s.owner.field_0x4be[1]=255;
}
int main(){hf.enabled=true;uf.enabled=true;
#ifdef HUD03_BASE
 Scene s;J2DOrthoGraph graph;lnk01::Visual v;v.alpha=1;hf.present.begin(true,true,v,lnk01::layout(0,0,608,448),{0,0,608,448});begin_owner(&s.owner);screen(s.root,graph);ck(s.root.getTypeID()==8&&root_disabled>0,"actual native type8 root must reproduce base regression");ck(!has(0)&&!has(1)&&!has(4)&&!has(9),"base loses protected HUD draw identities");std::printf("BASE_0_11_4_REGRESSION_PROVEN checks=%u root_disabled=%u protected_draws_lost=10 native_game=NOT_EXECUTED\n",checks,root_disabled);return 0;
#else
 unsigned cases=0;std::uint64_t protected_draws=0,lower_hidden=0;
 for(unsigned f=0;f<528;++f){Scene s;J2DOrthoGraph graph;const float width=f%2?796.444f:608;graph.ortho.i.x=(608-width)*.5f;graph.ortho.f.x=graph.ortho.i.x+width;
  const unsigned c=f%22;code(s,c);const bool second=f%7==0&&c!=21;s.owner.field_0x4be[1]=second?21:255;if(second)s.owner.mpButtonBin=&s.icon_manager[1];
  if(f%11==0){s.text[1].place(240,382,100,24);s.icon[1].place(350,382,24,24);}
  if(f%13==0){s.text[0].place(-100,375,100,24);s.icon[0].place(-75,375,24,24);}
  lnk01::Visual v;v.alpha=f%4==0?0:f%4==1?1:f%4==2?.2f:.005f;
  hf.present.begin(true,true,v,lnk01::layout(graph.ortho.i.x,0,width,448),{graph.ortho.i.x,0,graph.ortho.f.x,448});
  const bool active=hf.present.on;hooks=false;draws.clear();screen(s.root,graph);const auto reference=draws;const bool expected[2]={native_conflict(s,0),second&&native_conflict(s,1)};hooks=true;draws.clear();begin_owner(&s.owner);screen(s.root,graph);
  const auto lower_draws=draws;draws.clear();screen(s.general,graph);for(int id=40;id<50;++id)ck(std::count(draws.begin(),draws.end(),id)==1,"separate native mpMeterDraw general HUD root preserved");draws=lower_draws;
  auto* own=ui03_owner();ck(own!=nullptr,"live lower packet owner");bool inv[2]={expected[0],expected[1]};if(active){ck(own->slot[0].valid&&own->slot[0].invade==inv[0],"slot0 matches independent native geometric oracle");ck((own->slot[1].valid&&own->slot[1].invade)==inv[1],"slot1 matches independent native geometric oracle");}
  for(unsigned id=0;id<10;++id){ck(std::count(draws.begin(),draws.end(),int(id))==std::count(reference.begin(),reference.end(),int(id)),"hearts/map/ITEMS/MAP/rupees/top-right/EHB/target preserved, including crossing foreign widget");++protected_draws;}
  for(unsigned slot=0;slot<2;++slot)for(unsigned j=0;j<5;++j){const int id=100+int(slot)*20+int(j);ck(has(id)==!(active&&inv[slot]),"only exact native lower text/shadow cluster suppressed");if(!has(id))++lower_hidden;}
  if(button_tags[c])for(unsigned j=0;j<2;++j)ck(has(110+int(j))==!(active&&inv[0]),"lower button/glow primitives follow same ownership");
  ck(root_disabled==0,"screen/root/multi-HUD container must never be suppressed");
  direct_picture(&s.protected_ui[9],200);direct_string(201);outfont(&s.protected_ui[8],202);ck(has(200)&&has(201)&&has(202),"foreign picture/string/outfont inside band never skipped");
  direct_picture(&s.glyph[0][0],210);ck(has(210)==!(active&&inv[0]&&button_tags[c]),"direct picture needs exact certified identity");
  outfont(&s.letters[0][0],211);ck(has(211)==!(active&&inv[0]),"outfont requires lower textbox certificate");
  {J2DScreen* root=&s.root;float rx=0,ry=0;const J2DGrafContext* gp=&graph;void* sa[]={&root,&rx,&ry,&gp};on_screen_pre_fix02(nullptr,sa,nullptr,nullptr);
   J2DPane* pane=&s.letters[0][0];float x=0,y=0;const J2DGrafContext* port=&graph;bool orth=true,enabled=true;void* pa[]={&pane,&x,&y,&port,&orth,&enabled};
   on_pane_pre_fix02(nullptr,pa,nullptr,nullptr);
   JUTFont font;JUTFont* fp=&font;const char* label="Shield attack";u32 n=13;bool first=true;float sx=260,sy=391,w=8,h=12;void* a[]={&fp,&sx,&sy,&w,&h,&label,&n,&first};
   on_string_pre_fix02(nullptr,a,nullptr,nullptr);ck(font.mColor1.a==(active&&inv[0]?0:255),"certified font emission alpha");
   const float advance=n*w;on_string_post_fix02(nullptr,a,nullptr,nullptr);ck(font.mColor1.a==255&&font.mColor2.a==255&&font.mColor3.a==255&&font.mColor4.a==255&&advance==104,"owned font exact restoration and native advance");
   on_pane_post_fix02(nullptr,pa,nullptr,nullptr);ck(enabled,"owned draw argument restored");on_screen_post_fix02(nullptr,sa,nullptr,nullptr);}
  const auto oldf=s.owner.field_0x2e8[0];draws.clear();pikari(s,graph,&s.owner.field_0x2e8[0]);ck(s.owner.field_0x2e8[0]==oldf+.25f,"original Pikari animation always executes");ck(has(300)==!(active&&inv[0]),"Pikari bound by exact owner renderer/frame pointer/slot");ck(has(390),"foreign sibling on Pikari screen protected");
  float foreign_frame=1;draws.clear();pikari(s,graph,&foreign_frame);ck(has(300)&&has(390)&&foreign_frame==1.25f,"same Pikari screen foreign upper-HUD call always preserved");
  ck(s.owner.field_0x4be[0]==c,"button/action code unchanged");on_prompt_post_fix02(nullptr,nullptr,nullptr,nullptr);ck(ui03_owner()==nullptr&&!uf.owner[0].live&&!uf.owner[0].n,"no borrowed owner after draw");ui02_end_frame();hf.present.end();
  ck(!uf.owner_depth&&!uf.screen_depth&&!uf.pane_depth&&!uf.font_depth&&!uf.fx_depth&&!uf.frame_screen_n,"no cookies/pointers survive presentation");++cases;
 }
 // A forged root-as-slot, wrong tags, unrelated owner, unknown pane and graph
 // must all preserve native HUD. This is intentionally counted as uncovered.
 for(unsigned bad=0;bad<4;++bad){Scene s;J2DOrthoGraph graph;lnk01::Visual v;v.alpha=1;hf.present.begin(true,true,v,lnk01::layout(0,0,608,448),{0,0,608,448});
  if(bad==0)s.text_manager[0].pane=&s.root;if(bad==1)s.text[0].mInfoTag=tag("info_n");if(bad==2)s.hud.mpEmpButton=nullptr;if(bad==3)s.owner.mpButtonScreen=&s.pikari;
  draws.clear();begin_owner(&s.owner);screen(s.root,graph);for(int id=0;id<10;++id)ck(has(id),"unclassified ownership never erases HUD");on_prompt_post_fix02(nullptr,nullptr,nullptr,nullptr);ui02_end_frame();hf.present.end();}
 ck(!root_disabled&&uf.pane_suppressed==uf.pane_restored&&!uf.unpaired,"zero root suppression and exact paired restores");
 ck(uf.roots_kept==uf.roots_seen&&lower_hidden>0,"meaningful protected/emission coverage");
 std::printf("HUD_FIX03_ADAPTER checks=%u failures=0 RESULT=LAB_PASS scenarios=%u protected_draws=%llu lower_text_draws_hidden=%llu root_suppressed=%u preserved_roots=%llu original_fx_updates=%u native_game=NOT_EXECUTED\n",checks,cases,(unsigned long long)protected_draws,(unsigned long long)lower_hidden,root_disabled,(unsigned long long)uf.roots_kept,fx_updates);
#endif
}
