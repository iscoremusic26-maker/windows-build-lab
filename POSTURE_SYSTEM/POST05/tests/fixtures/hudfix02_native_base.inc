struct HudFixGlue {
  hudfix01::Presentation present;
  hudfix01::Scene scene;
  J2DPane* panes[hudfix01::kNodes]{};
  struct Saved { J2DPane* pane=nullptr;float tx=0,ty=0,mx=0,my=0; } saved[hudfix01::kNodes]{};
  unsigned saved_n=0,depth=0,restore_depth=0;
  bool enabled=false;
  GfxStageHookHandle before=0,after=0;
  std::uint64_t screens=0,moved=0,restored=0,unsupported=0,overflow=0,unpaired=0,remaining=0;
  std::uint64_t offered=0,offered_frames=0,sword_contacts=0,last_sword=0,near_sword_off=0;
  unsigned lines=0;
} hf;

void hudfix_restore() noexcept {
  while(hf.saved_n) {
    const HudFixGlue::Saved& s=hf.saved[--hf.saved_n];
    J2DPane* pane=s.pane;
    pane->mTranslateX=s.tx;pane->mTranslateY=s.ty;
    pane->mPositionMtx[0][3]=s.mx;pane->mPositionMtx[1][3]=s.my;
    hf.saved[hf.saved_n].pane=nullptr;++hf.restored;
  }
}
hudfix01::Matrix hudfix_matrix(const Mtx& m) noexcept {
  hudfix01::Matrix a;for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)a.v[y][x]=m[y][x];return a;
}
// Exactly the PC J2DPane root calculation, including the 608-wide authored
// canvas, pivot, scale and rotations. Virtual makeMatrix is the live pane's
// native renderer method; every authored field/matrix is restored immediately.
hudfix01::Matrix hudfix_root(J2DPane* pane,float x,float y,float width) noexcept {
  const hudfix01::Matrix saved=hudfix_matrix(pane->mPositionMtx);
  const float sx=pane->mScaleX,tx=pane->mTranslateX;
  if(pane->mUserInfoTag!=0x6e5f3433ull) {
    const float f=width/608.0f;pane->mScaleX=sx*f;pane->mTranslateX=f*(tx-304.0f)+304.0f;
  }
  pane->makeMatrix(pane->mTranslateX+x,pane->mTranslateY+y);
  const hudfix01::Matrix result=hudfix_matrix(pane->mPositionMtx);
  pane->mScaleX=sx;pane->mTranslateX=tx;
  for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)pane->mPositionMtx[r][c]=saved.v[r][c];
  return result;
}
void hudfix_collect(J2DPane* pane,int parent,const hudfix01::Matrix& world,bool visible,unsigned alpha,unsigned depth) noexcept {
  if(!pane||depth>=hudfix01::kDepth||hf.scene.n>=hudfix01::kNodes){hf.scene.complete=false;return;}
  const auto& b=pane->mBounds;
  hudfix01::Rect bounds{b.i.x,b.i.y,b.f.x,b.f.y};
  if(!bounds.valid())return; // native J2DPane::draw also omits this entire subtree
  const unsigned i=hf.scene.n++;
  hf.panes[i]=pane;hf.scene.nodes[i]=hudfix01::Node{};
  auto& n=hf.scene.nodes[i];n.parent=parent;n.world=world;
  const bool shown=visible&&pane->mVisible;
  const unsigned opacity=parent<0||!pane->mIsInfluencedAlpha?pane->mAlpha:(alpha*pane->mAlpha)/255;
  const unsigned type=pane->getTypeID();
  if(shown&&opacity&&type!=16&&type!=17)n.own=hudfix01::grow(hudfix01::transformed(bounds,world),6.0f);
  for(JSUTreeIterator<J2DPane> it=pane->mPaneTree.getFirstChild();it!=pane->mPaneTree.getEndChild();++it) {
    if(!hf.scene.complete)break;
    J2DPane* child=it.getObject();
    auto local=hudfix_matrix(child->mPositionMtx);
    if(child->mUserInfoTag==0x6e5f3433ull)for(unsigned c=0;c<3;++c)local.v[0][c]*=608.0f/(hf.present.viewport.x1-hf.present.viewport.x0);
    hudfix_collect(child,static_cast<int>(i),hudfix01::multiply(world,local),shown,opacity,depth+1);
  }
}
HookAction on_screen_pre_hudfix(ModContext*,void* args,void*,void*) noexcept {
  ++hf.depth;
  if(!hf.enabled||!hf.present.inside||!hf.present.on||!args)return HOOK_CONTINUE;
  if(hf.depth!=1){++hf.overflow;return HOOK_CONTINUE;}
  J2DScreen* screen=mods::arg<J2DScreen*>(args,0);
  const auto* graph=mods::arg<const J2DGrafContext*>(args,3);
  if(!screen||!graph||graph->getGrafType()!=1||screen->mScissor){++hf.unsupported;return HOOK_CONTINUE;}
  // Only the current native ortho port is supported. Other camera/viewport
  // contexts are left untouched and counted, never guessed or globally hidden.
  const auto* ortho=static_cast<const J2DOrthoGraph*>(graph);
  const auto* o=ortho->getOrtho();
  const auto v=hf.present.viewport;
  if(hudfix01::absf(o->i.x-v.x0)>0.05f||hudfix01::absf(o->i.y-v.y0)>0.05f||hudfix01::absf(o->f.x-v.x1)>0.05f||hudfix01::absf(o->f.y-v.y1)>0.05f) {
    ++hf.unsupported;return HOOK_CONTINUE;
  }
  ++hf.screens;hf.scene.clear();
  const float x=mods::arg<float>(args,1),y=mods::arg<float>(args,2);
  hudfix_collect(screen,-1,hudfix_root(screen,x,y,v.x1-v.x0),true,255,0);
  if(!hf.scene.complete){++hf.overflow;return HOOK_CONTINUE;}
  hf.scene.plan(hf.present.band,v);
  // Validate the complete lease before the first write. Root movement is passed
  // as draw arguments; descendants use the inverse parent transform.
  for(unsigned i=1;i<hf.scene.n;++i)if(hf.scene.nodes[i].selected) {
    auto& n=hf.scene.nodes[i];float lx=0,ly=0;
    if(!hudfix01::local_delta(hf.scene.nodes[n.parent].world,n.dx,n.dy,lx,ly)) {++hf.unsupported;return HOOK_CONTINUE;}
  }
  hf.restore_depth=hf.depth;
  for(unsigned i=0;i<hf.scene.n;++i) {
    auto& n=hf.scene.nodes[i];if(!n.selected)continue;
    ++hf.moved;
    if(i==0) {mods::arg_ref<float>(args,1)=x+n.dx;mods::arg_ref<float>(args,2)=y+n.dy;continue;}
    J2DPane* pane=hf.panes[i];float lx=0,ly=0;
    (void)hudfix01::local_delta(hf.scene.nodes[n.parent].world,n.dx,n.dy,lx,ly);
    hf.saved[hf.saved_n++]={pane,pane->mTranslateX,pane->mTranslateY,pane->mPositionMtx[0][3],pane->mPositionMtx[1][3]};
    pane->mTranslateX+=lx;pane->mTranslateY+=ly;
    pane->mPositionMtx[0][3]+=lx;pane->mPositionMtx[1][3]+=ly;
  }
  // Geometry verdict over ALL renderable leaves, including descendants whose
  // parent moved. This is a model bound, not a screenshot or raster proof.
  for(unsigned i=0;i<hf.scene.n;++i) {
    float dx=0,dy=0;for(int p=static_cast<int>(i);p>=0;p=hf.scene.nodes[p].parent)if(hf.scene.nodes[p].selected){dx=hf.scene.nodes[p].dx;dy=hf.scene.nodes[p].dy;break;}
    if(hudfix01::intersects(hudfix01::shift(hf.scene.nodes[i].own,dx,dy),hf.present.band))++hf.remaining;
  }
  return HOOK_CONTINUE;
}
void on_screen_post_hudfix(ModContext*,void*,void*,void*) noexcept {
  if(!hf.depth){++hf.unpaired;return;}
  if(hf.depth==hf.restore_depth) {hudfix_restore();hf.restore_depth=0;}
  --hf.depth;
  // No native pointer is retained after the outer draw, even on refusal.
  if(!hf.depth)for(unsigned i=0;i<hf.scene.n;++i)hf.panes[i]=nullptr;
}

// FIX02: conditional visual suppression. No reflow, mVisible writes, button
// status writes or gameplay calls. Pane draw continues to update native caches;
// only its by-value draw-enable argument is leased, then restored in POST.
struct UiFix02 {
  struct Context {std::uintptr_t keys[hudfix01::kNodes]{};bool mask[hudfix01::kNodes]{};unsigned n=0;} context[4];
  struct PaneCall {bool changed=false,visible=false;} pane_call[64];
  struct FontCall {JUtility::TColor colors[4];bool changed=false;} font_call[4];
  dMeterButton_c* owner[4]{};
  unsigned screen_depth=0,pane_depth=0,font_depth=0,owner_depth=0,text_n=0,cluster_n=0;
  std::uintptr_t texts[256]{};hudfix01::Rect clusters[32]{};
  std::uintptr_t frame_screens[128]{};unsigned frame_screen_n=0,frame_screen_calls=0;
  std::uint64_t observed_frame=0,screen_repeats=0,screen_calls_frame_max=0;
  bool enabled=false;
  std::uint64_t screens=0,panes=0,eligible=0,clusters_seen=0,candidates=0,masked=0,pane_suppressed=0,pane_restored=0;
  std::uint64_t pictures=0,picture_suppressed=0,outfonts=0,outfont_suppressed=0,strings=0,string_suppressed=0,font_restored=0;
  std::uint64_t kept=0,unknown=0,overflow=0,unpaired=0,uncovered=0;
} uf;
void ui02_end_frame() noexcept {
  for(unsigned i=0;i<uf.frame_screen_n;++i)uf.frame_screens[i]=0;
  uf.frame_screen_n=uf.frame_screen_calls=0;
}
void ui02_screen_observed(J2DScreen* screen) noexcept {
  if(uf.observed_frame!=hf.present.serial){ui02_end_frame();uf.observed_frame=hf.present.serial;}
  ++uf.frame_screen_calls;if(uf.frame_screen_calls>uf.screen_calls_frame_max)uf.screen_calls_frame_max=uf.frame_screen_calls;
  const auto key=reinterpret_cast<std::uintptr_t>(screen);
  for(unsigned i=0;i<uf.frame_screen_n;++i)if(uf.frame_screens[i]==key){++uf.screen_repeats;return;}
  if(uf.frame_screen_n<128)uf.frame_screens[uf.frame_screen_n++]=key;else {++uf.overflow;++uf.uncovered;}
}
bool ui02_active() noexcept {return uf.enabled&&hf.present.inside&&hf.present.on;}
bool ui02_conflict(hudfix01::Rect r) noexcept {
  if(hudfix01::intersects(r,hf.present.band))return true;
  for(unsigned i=0;i<uf.cluster_n;++i)if(hudfix01::intersects(r,uf.clusters[i]))return true;
  return false;
}
void ui02_text(std::uintptr_t key) noexcept {
  for(unsigned i=0;i<uf.text_n;++i)if(uf.texts[i]==key)return;
  if(uf.text_n<256)uf.texts[uf.text_n++]=key;else {++uf.overflow;++uf.uncovered;}
}
void ui02_cluster(hudfix01::Rect r) noexcept {
  if(uf.cluster_n<32)uf.clusters[uf.cluster_n++]=r;else {++uf.overflow;++uf.uncovered;}
}
HookAction on_prompt_pre_fix02(ModContext*,void* args,void*,void*) noexcept {
  if(uf.owner_depth<4)uf.owner[uf.owner_depth]=args?mods::arg<dMeterButton_c*>(args,0):nullptr;else ++uf.overflow;
  ++uf.owner_depth;return HOOK_CONTINUE;
}
void on_prompt_post_fix02(ModContext*,void*,void*,void*) noexcept {
  if(!uf.owner_depth){++uf.unpaired;return;}--uf.owner_depth;if(uf.owner_depth<4)uf.owner[uf.owner_depth]=nullptr;
}
hudfix01::Rect ui02_bounds(J2DPane* root) noexcept {
  for(unsigned i=0;i<hf.scene.n;++i)if(hf.panes[i]==root)return hf.scene.nodes[i].all;
  return {};
}
void ui02_mark(J2DPane* root,UiFix02::Context& ctx) noexcept {
  if(!root)return;
  for(unsigned i=0;i<hf.scene.n;++i) {
    bool under=false;for(int p=static_cast<int>(i);p>=0;p=hf.scene.nodes[p].parent)if(hf.panes[p]==root){under=true;break;}
    if(under)ctx.mask[i]=true;
  }
}
HookAction on_screen_pre_fix02(ModContext*,void* args,void*,void*) noexcept {
  const unsigned depth=uf.screen_depth++;
  if(depth>=4){++uf.overflow;++uf.uncovered;return HOOK_CONTINUE;}
  auto& ctx=uf.context[depth];ctx.n=0;
  if(!ui02_active()||!args)return HOOK_CONTINUE;
  J2DScreen* screen=mods::arg<J2DScreen*>(args,0);const auto* graph=mods::arg<const J2DGrafContext*>(args,3);
  if(!screen||!graph||graph->getGrafType()!=1){++uf.unknown;++uf.uncovered;return HOOK_CONTINUE;}
  const auto* o=static_cast<const J2DOrthoGraph*>(graph)->getOrtho();const auto v=hf.present.viewport;
  if(hudfix01::absf(o->i.x-v.x0)>.05f||hudfix01::absf(o->i.y-v.y0)>.05f||hudfix01::absf(o->f.x-v.x1)>.05f||hudfix01::absf(o->f.y-v.y1)>.05f){++uf.unknown;++uf.uncovered;return HOOK_CONTINUE;}
  ++uf.screens;ui02_screen_observed(screen);hf.scene.clear();
  hudfix_collect(screen,-1,hudfix_root(screen,mods::arg<float>(args,1),mods::arg<float>(args,2),v.x1-v.x0),true,255,0);
  if(!hf.scene.complete){++uf.overflow;++uf.uncovered;return HOOK_CONTINUE;}
  hf.scene.aggregate();ctx.n=hf.scene.n;
  for(unsigned i=0;i<ctx.n;++i){ctx.keys[i]=reinterpret_cast<std::uintptr_t>(hf.panes[i]);ctx.mask[i]=hudfix01::intersects(hf.scene.nodes[i].own,hf.present.band);}
  // Native action slots: all22 button kinds, never action-name strings. trans_button
  // has run inside dMeterButton::draw before this screen call, so matrices are current.
  dMeterButton_c* b=uf.owner_depth&&uf.owner_depth<=4?uf.owner[uf.owner_depth-1]:nullptr;
  if(b&&screen==b->mpButtonScreen) {
    CPaneMgr* buttons[22]={b->mpButtonA,b->mpButtonB,b->mpButtonR,b->mpButtonZ,b->mpButton3D,b->mpButtonC,b->mpButtonS,b->mpButtonX,b->mpButtonY,
      b->mpButtonNun,b->mpButtonRemo,b->mpButtonRemo2,b->mpButtonAR,b->mpButton3DB,b->mpButtonNURE,b->mpButtonReel,b->mpButtonReel2,b->mpButtonAB,b->mpButtonTate,b->mpButtonNunZ,b->mpButtonNunC,b->mpButtonBin};
    for(unsigned slot=0;slot<2;++slot) {
      const unsigned code=b->field_0x4be[slot];if(code>=22||!b->mpText[slot])continue;
      ++uf.clusters_seen;J2DPane* text=b->mpText[slot]->getPanePtr();J2DPane* icon=buttons[code]?buttons[code]->getPanePtr():nullptr;
      const auto bounds=hudfix01::unite(ui02_bounds(text),ui02_bounds(icon));
      if(!hudfix01::intersects(bounds,hf.present.band))continue;
      ++uf.candidates;ui02_mark(text,ctx);ui02_mark(icon,ctx);ui02_cluster(hudfix01::grow(bounds,4));
    }
  }
  const dMeter2_c* meter=dMeter2Info_getMeterClass();
  const bool pikari=meter&&meter->mpMeterDraw&&screen==meter->mpMeterDraw->mpPikariScreen;
  if(pikari&&ui02_conflict(hf.scene.nodes[0].all))for(unsigned i=0;i<ctx.n;++i)ctx.mask[i]=true;
  for(unsigned i=0;i<ctx.n;++i) {
    if(ctx.mask[i]){++uf.masked;if(hf.panes[i]->getTypeID()==19)ui02_text(ctx.keys[i]);}
    hf.panes[i]=nullptr;
  }
  return HOOK_CONTINUE;
}
void on_screen_post_fix02(ModContext*,void*,void*,void*) noexcept {
  for(unsigned i=0;i<hudfix01::kNodes;++i)hf.panes[i]=nullptr; // includes refused/overflow collection
  if(!uf.screen_depth){++uf.unpaired;return;}--uf.screen_depth;
  if(uf.screen_depth<4){auto& c=uf.context[uf.screen_depth];for(unsigned i=0;i<c.n;++i)c.keys[i]=0;c.n=0;}
}
HookAction on_pane_pre_fix02(ModContext*,void* args,void*,void*) noexcept {
  const unsigned depth=uf.pane_depth++;if(depth>=64){++uf.overflow;++uf.uncovered;return HOOK_CONTINUE;}
  auto& call=uf.pane_call[depth];call.changed=false;
  if(!ui02_active()||!args)return HOOK_CONTINUE;
  ++uf.panes;J2DPane* pane=mods::arg<J2DPane*>(args,0);
  if(!pane||!mods::arg<bool>(args,5)||!pane->mVisible||!pane->mAlpha)return HOOK_CONTINUE;
  ++uf.eligible;bool suppress=false,observed=false;
  if(uf.screen_depth&&uf.screen_depth<=4) {
    const auto& c=uf.context[uf.screen_depth-1];const auto key=reinterpret_cast<std::uintptr_t>(pane);
    for(unsigned i=0;i<c.n;++i)if(c.keys[i]==key){suppress=c.mask[i];observed=true;break;}
  }
  if(!observed&&pane->getTypeID()!=16&&pane->getTypeID()!=17) {
    const auto* graph=mods::arg<const J2DGrafContext*>(args,3);
    if(graph&&graph->getGrafType()==1) {
      hudfix01::Matrix matrix;
      if(auto* parent=pane->mPaneTree.getParent())matrix=hudfix01::multiply(hudfix_matrix(parent->getObject()->mGlobalMtx),hudfix_matrix(pane->mPositionMtx));
      else matrix=hudfix_root(pane,mods::arg<float>(args,1),mods::arg<float>(args,2),hf.present.viewport.x1-hf.present.viewport.x0);
      const auto& b=pane->mBounds;suppress=hudfix01::intersects(hudfix01::grow(hudfix01::transformed({b.i.x,b.i.y,b.f.x,b.f.y},matrix),6),hf.present.band);observed=true;
    }else {++uf.unknown;++uf.uncovered;}
  }
  if(suppress){call.visible=mods::arg<bool>(args,5);call.changed=true;mods::arg_ref<bool>(args,5)=false;++uf.pane_suppressed;}
  else if(observed)++uf.kept;
  return HOOK_CONTINUE;
}
void on_pane_post_fix02(ModContext*,void* args,void*,void*) noexcept {
  if(!uf.pane_depth){++uf.unpaired;return;}--uf.pane_depth;
  if(uf.pane_depth<64){auto& c=uf.pane_call[uf.pane_depth];if(c.changed&&args){mods::arg_ref<bool>(args,5)=c.visible;c.changed=false;++uf.pane_restored;}}
}
HookAction on_picture_pre_fix02(ModContext*,void* args,void*,void*) noexcept {
  if(!ui02_active()||!args)return HOOK_CONTINUE;++uf.pictures;
  const float x=mods::arg<float>(args,1),y=mods::arg<float>(args,2),w=mods::arg<float>(args,3),h=mods::arg<float>(args,4);
  if(hudfix01::intersects(hudfix01::grow({x,y,x+w,y+h},3),hf.present.band)){++uf.picture_suppressed;return HOOK_SKIP_ORIGINAL;}
  ++uf.kept;return HOOK_CONTINUE;
}
HookAction on_outfont_pre_fix02(ModContext*,void* args,void*,void*) noexcept {
  if(!ui02_active()||!args)return HOOK_CONTINUE;++uf.outfonts;
  const auto key=reinterpret_cast<std::uintptr_t>(mods::arg<J2DTextBox*>(args,1));
  for(unsigned i=0;i<uf.text_n;++i)if(uf.texts[i]==key){++uf.outfont_suppressed;return HOOK_SKIP_ORIGINAL;}
  ++uf.kept;return HOOK_CONTINUE;
}
HookAction on_string_pre_fix02(ModContext*,void* args,void*,void*) noexcept {
  const unsigned depth=uf.font_depth++;if(depth>=4){++uf.overflow;++uf.uncovered;return HOOK_CONTINUE;}
  auto& lease=uf.font_call[depth];lease.changed=false;
  if(!ui02_active()||!args)return HOOK_CONTINUE;++uf.strings;
  JUTFont* font=mods::arg<JUTFont*>(args,0);const char* text=mods::arg<const char*>(args,5);unsigned n=mods::arg<u32>(args,6);
  if(!font||!text||n>512){++uf.unknown;++uf.uncovered;return HOOK_CONTINUE;}
  const float cell=static_cast<float>(font->getCellWidth()),height=static_cast<float>(font->getHeight());
  if(!(cell>0&&height>0)){++uf.unknown;++uf.uncovered;return HOOK_CONTINUE;}
  float x=mods::arg<float>(args,1);const float y=mods::arg<float>(args,2),w=mods::arg<float>(args,3),h=mods::arg<float>(args,4);
  bool first=mods::arg<bool>(args,7);hudfix01::Rect bounds;
  for(unsigned i=0;i<n;++i) {
    int code=static_cast<unsigned char>(text[i]);if(font->isLeadByte(code)&&i+1<n)code=(code<<8)|static_cast<unsigned char>(text[++i]);
    JUTFont::TWidth glyph{};font->getWidthEntry(code,&glyph);
    const float x0=font->isFixed()||!first?x:x-glyph.field_0x0*w/cell;
    bounds=hudfix01::unite(bounds,{x0,y-font->getAscent()*h/height,x0+w,y+font->getDescent()*h/height});
    x+=(font->isFixed()?font->getFixedWidth():glyph.field_0x1+(first?0:glyph.field_0x0))*w/cell;first=true;
  }
  if(hudfix01::intersects(hudfix01::grow(bounds,2),hf.present.band)) {
    lease.colors[0]=font->mColor1;lease.colors[1]=font->mColor2;lease.colors[2]=font->mColor3;lease.colors[3]=font->mColor4;lease.changed=true;
    font->mColor1.a=0;font->mColor2.a=0;font->mColor3.a=0;font->mColor4.a=0;++uf.string_suppressed;
  }else ++uf.kept;
  return HOOK_CONTINUE; // execute original: exact native text advance/return value is preserved
}
void on_string_post_fix02(ModContext*,void* args,void*,void*) noexcept {
  if(!uf.font_depth){++uf.unpaired;return;}--uf.font_depth;if(uf.font_depth>=4)return;
  auto& lease=uf.font_call[uf.font_depth];if(lease.changed&&args) {
    JUTFont* font=mods::arg<JUTFont*>(args,0);font->mColor1=lease.colors[0];font->mColor2=lease.colors[1];font->mColor3=lease.colors[2];font->mColor4=lease.colors[3];lease.changed=false;++uf.font_restored;
  }
}

