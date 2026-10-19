using System.Text.Json;
namespace DevfMap;
public sealed class MultiEditor:Form{
 readonly Dictionary<string,ResourceInfo> observed=new();
 MultiDocument data=MultiDocument.Empty();readonly Dictionary<string,PlacementGeometry> loaded=new();
 readonly Canvas canvas=new(){Dock=DockStyle.Fill};readonly Projection view=new();readonly ListBox list=new(){Width=320,Height=260,HorizontalScrollbar=true};
 readonly ComboBox rooms=new(){Width=380,DropDownWidth=640,DropDownStyle=ComboBoxStyle.DropDownList},tool=new(){Width=160,DropDownStyle=ComboBoxStyle.DropDownList},layer=new(){Width=300,DropDownStyle=ComboBoxStyle.DropDownList};
 readonly CheckBox enabled=new(){Text="Habilitar para QA en arranque frío",Width=310};
 // CP20 explicit resources; legacy Blue E_OC2 is displayed separately and never converted on selection.
 readonly ComboBox variantBox=new(){Width=300,DropDownStyle=ComboBoxStyle.DropDownList};
 readonly NumericUpDown hpBox=new(){Minimum=MultiConfig.HpMin,Maximum=MultiConfig.HpMax,Value=MultiConfig.HpDefaultBlack,Width=180,Enabled=false};
 readonly NumericUpDown yaw=new(){Minimum=-32768,Maximum=32767,Width=180};
 readonly Label status=new(){Dock=DockStyle.Bottom,Height=100,Padding=new Padding(8)},detail=new(){Width=315,Height=150};
 internal Func<MultiEncounter,MultiEncounter,bool>? ConfirmVariantForTest;
 internal Func<string?>? BackupPathForTest;
 string? path,expected;bool dirty,updating,pan;Point prior,moveStart;int moving=-1;Bitmap? raster;
 PlacementGeometry? current=>rooms.SelectedItem is RoomChoice choice&&loaded.TryGetValue(choice.Key,out var g)?g:null;
 // CP10.1: the selector holds a stable technical key; the English label is display only and never used for lookup.
 internal sealed record RoomChoice(string Key,string Display){public override string ToString()=>Display;}
 void AddRoom(PlacementGeometry g){if(rooms.Items.Cast<RoomChoice>().Any(c=>c.Key==g.Permit.key))return;rooms.Items.Add(new RoomChoice(g.Permit.key,NameResolver.Resolve(g.Permit.stage,g.Permit.room,g.Permit.variant).Full));}
 void SelectRoom(string key){for(int i=0;i<rooms.Items.Count;i++)if(rooms.Items[i] is RoomChoice c&&c.Key==key){rooms.SelectedIndex=i;return;}}
 string PlaceOf(MultiEncounter e)=>loaded.TryGetValue(e.resource,out var g)?NameResolver.Resolve(g.Permit.stage,g.Permit.room,g.Permit.variant).Place:NameResolver.Resolve(e.stage,e.room,"00").Place;
 public int ModelCount=>data.encounters.Count;
 public MultiEditor(){
 Text="CP20 · F_SP121 · catálogo CP13 · TEST privado";Width=1350;Height=900;MinimumSize=new(1000,700);
 var top=new FlowLayoutPanel{Dock=DockStyle.Top,AutoSize=true,Padding=new Padding(5)};
 void Button(string name,Action action){var b=new Button{Text=name,AutoSize=true};b.Click+=(_,_)=>Run(action);top.Controls.Add(b);}
 Button("Compatibilidad CP08",()=>new Editor().Show(this));
 Button("CISO / Catálogo",()=>new CatalogBrowser(raw=>{var e=new Editor();e.AcceptImported(raw);e.Show(this);},Accept,ObserveCatalog,PrepareBlocked).Show(this));
 Button("Abrir schema2",Open);Button("Copiar legacy schema1…",ImportLegacy);Button("Guardar prueba…",Save);
 Button("Instalar JSON… (juego cerrado)",Install);Button("Restaurar backup / Abrir",Open);
 Button("Descartar",()=>{if(!Discard())return;if(path!=null){data=MultiConfig.Decode(MultiConfig.Read(path));expected=MultiConfig.HashFile(path);}else data=MultiDocument.Empty();dirty=false;RefreshList();});
 Button("Nuevo",()=>{if(!Discard())return;data=MultiDocument.Empty();path=expected=null;dirty=false;RefreshList();});
 Button("Preparar sala bloqueada…",()=>{if(rooms.SelectedItem is RoomChoice c&&observed.TryGetValue(c.Key,out var r))PrepareBlocked(r);});Button("Encuadrar",Frame);top.Controls.Add(rooms);tool.Items.AddRange(new object[]{"Seleccionar / mover","Añadir","Desplazar"});tool.SelectedIndex=0;top.Controls.Add(tool);
 var side=new FlowLayoutPanel{Dock=DockStyle.Right,Width=340,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Padding=new Padding(8)};
 side.Controls.Add(new Label{Width=315,Height=75,Text="Blue E_OC · Red/Black E_OC2 · 32 global\nF_SP121 / catálogo CP13\nRequiere Provider 0.14.0 + Expansion 0.16.3\nPrueba humana pendiente"});
 side.Controls.Add(list);for(int i=0;i<15;i++)layer.Items.Add("Capa "+i+" · candidata QA");side.Controls.Add(layer);side.Controls.Add(enabled);variantBox.Items.AddRange(new object[]{"Blue auténtico (E_OC) · 220 HP","Red (E_OC2) · 330 HP","Black (E_OC2) · HP configurable","Blue heredado (E_OC2) · documento existente"});variantBox.SelectedIndex=0;side.Controls.Add(new Label{Text="Variant · HP (Black only, 1–999)",AutoSize=true});side.Controls.Add(variantBox);side.Controls.Add(hpBox);side.Controls.Add(new Label{Text="Yaw s16",AutoSize=true});side.Controls.Add(yaw);side.Controls.Add(detail);
 void Act(string n,Action a){var b=new Button{Text=n,Width=310};b.Click+=(_,_)=>Run(a);side.Controls.Add(b);}
 Act("Duplicar en un nuevo punto",()=>{MultiConfig.C(list.SelectedIndex>=0,"Seleccione un encuentro");tool.SelectedIndex=1;status.Text="Clic en otro suelo: nuevo ID, desactivado. No se duplica en la misma posición.";});
 Act("Eliminar",()=>{int i=list.SelectedIndex;if(i>=0&&MessageBox.Show(this,"¿Eliminar del borrador?","Confirmar",MessageBoxButtons.YesNo)==DialogResult.Yes){data.encounters.RemoveAt(i);Changed();}});
 side.Controls.Add(new Label{Width=315,Height=160,Text="Nuevos puntos: disabled por defecto.\nActivación QA explícita; no prueba capa activa.\nClic: añadir/mover. Derecha: desplazar.\nRueda: zoom. Ningún cambio en vivo.\nLos recursos solo se importan localmente.\nGuardar exige volver a cargar sus mapas."});
 Controls.Add(canvas);Controls.Add(side);Controls.Add(top);Controls.Add(status);
 rooms.SelectedIndexChanged+=(_,_)=>Run(()=>{if(!updating){list.SelectedIndex=-1;layer.SelectedIndex=-1;}raster?.Dispose();raster=current==null?null:CatalogBrowser.Rasterize(current.Mesh,current.Permit.room,CancellationToken.None);Frame();if(current==null&&rooms.SelectedItem is RoomChoice c&&observed.TryGetValue(c.Key,out var r))status.Text=Coverage.Reason(r);});
 list.SelectedIndexChanged+=(_,_)=>SelectItem();
 enabled.CheckedChanged+=(_,_)=>{if(updating||list.SelectedIndex<0)return;Run(()=>{int i=list.SelectedIndex;var e=data.encounters[i];bool on=enabled.Checked;if(on){CheckResource(e);if(e.layer_state!="CP08_OBSERVED"&&MessageBox.Show(this,$"Sala {e.room}, capa {e.layer}: NO VERIFICADO EN RUNTIME. ¿Habilitar solo para QA en arranque frío?","Autorización QA",MessageBoxButtons.YesNo)!=DialogResult.Yes){SelectItem();return;}}Replace(i,e with{enabled=on,qa_confirmed=on||e.qa_confirmed});});};
 layer.SelectedIndexChanged+=(_,_)=>{if(updating||list.SelectedIndex<0||layer.SelectedIndex<0)return;Run(()=>{int i=list.SelectedIndex;var e=data.encounters[i];if(e.layer!=layer.SelectedIndex)Replace(i,e with{layer=layer.SelectedIndex,enabled=false,qa_confirmed=false,layer_state="QA_CANDIDATE"});});};
 variantBox.SelectedIndexChanged+=(_,_)=>{hpBox.Enabled=variantBox.SelectedIndex==2;if(updating||list.SelectedIndex<0)return;Run(()=>{
  int i=list.SelectedIndex;var e=data.encounters[i];string? want=SelectedVariant;
  if((e.IsLegacy&&want==null)||e.bokoblin_variant==want)return;
  MultiConfig.C(want!=null,"Blue heredado solo se conserva en documentos existentes; seleccione una variante explícita.");
  var changed=VariantEditing.Change(e,want!,want=="black"?(int)hpBox.Value:null);
  if(!(ConfirmVariantForTest?.Invoke(e,changed)??(MessageBox.Show(this,$"{e.VariantLabel} → {changed.VariantLabel}. Recurso {e.enemy} → {changed.enemy}. Se conserva ID/posición/yaw, se desactiva y exige nueva QA. Se guardará una copia previa. ¿Convertir explícitamente?","Conversión de variante",MessageBoxButtons.YesNo)==DialogResult.Yes))){SelectItem();return;}
  CheckResource(e);CheckResource(changed);
  string backup;
  if(path!=null){MultiConfig.C(MultiConfig.HashFile(path)==expected,"EXTERNAL_FILE_CHANGED");backup=path+".conversion-"+Guid.NewGuid().ToString("N")+".backup.json";}
  else if(BackupPathForTest!=null){var chosen=BackupPathForTest();if(chosen==null){SelectItem();return;}backup=chosen;}
  else {using var copy=new SaveFileDialog{Filter="Copia anterior|*.json",FileName="encounters.before-conversion.json",OverwritePrompt=true};if(copy.ShowDialog(this)!=DialogResult.OK){SelectItem();return;}backup=copy.FileName;}
  VariantEditing.Backup(backup,path!=null?MultiConfig.Read(path):MultiConfig.Encode(data));Replace(i,changed);
 });};
 hpBox.ValueChanged+=(_,_)=>{if(updating||list.SelectedIndex<0||variantBox.SelectedIndex!=2)return;Run(()=>{int i=list.SelectedIndex;var e=data.encounters[i];if(e.hp!=(int)hpBox.Value)Replace(i,e with{hp=(int)hpBox.Value});});};
 yaw.ValueChanged+=(_,_)=>{if(!updating&&list.SelectedIndex>=0)Run(()=>{int i=list.SelectedIndex;Replace(i,data.encounters[i] with{yaw_s16=(int)yaw.Value,enabled=false,qa_confirmed=false,layer_state="QA_CANDIDATE"});});};
 canvas.Paint+=Paint;canvas.MouseEnter+=(_,_)=>canvas.Focus();
 canvas.MouseWheel+=(_,e)=>{var a=view.World(e.Location,canvas.Size);view.Scale=Math.Clamp(view.Scale*(e.Delta>0?1.2:1/1.2),.00001,10);var b=view.World(e.Location,canvas.Size);view.CenterX+=a.X-b.X;view.CenterZ+=a.Z-b.Z;canvas.Invalidate();};
 canvas.MouseDown+=(_,e)=>Run(()=>{if(e.Button==MouseButtons.Right||tool.SelectedIndex==2){pan=true;prior=e.Location;canvas.Capture=true;return;}if(e.Button!=MouseButtons.Left||current==null)return;
 if(tool.SelectedIndex==1){MultiConfig.C(layer.SelectedIndex>=0,"Seleccione explícitamente una capa candidata (0–14)");var p=view.World(e.Location,canvas.Size);Add(p.X,p.Z);return;}
 moving=Hit(e.Location);moveStart=e.Location;list.SelectedIndex=moving;if(moving>=0)canvas.Capture=true;});
 canvas.MouseMove+=(_,e)=>{if(pan){view.CenterX-=(e.X-prior.X)/view.Scale;view.CenterZ+=(e.Y-prior.Y)/view.Scale;prior=e.Location;canvas.Invalidate();}};
 canvas.MouseUp+=(_,e)=>{if(moving>=0&&current!=null&&Math.Abs(e.X-moveStart.X)+Math.Abs(e.Y-moveStart.Y)>=4){int i=moving;Run(()=>{var p=view.World(e.Location,canvas.Size);var old=data.encounters[i];var y=current.Place(p.X,p.Z);if(p.X!=(float)old.x||p.Z!=(float)old.z)Replace(i,old with{x=p.X,y=y,z=p.Z,enabled=false,qa_confirmed=false,layer_state="QA_CANDIDATE"});});}moving=-1;pan=false;canvas.Capture=false;};
 FormClosing+=(_,e)=>{if(!Discard())e.Cancel=true;};FormClosed+=(_,_)=>raster?.Dispose();
 status.Text="Importe el CISO probado y seleccione un recurso F_SP121. Todas las salas presentes: seleccionables. Geometría y capa se validan por recurso; ninguna sala nueva se certifica automáticamente.";
 }
 string? SelectedVariant=>variantBox.SelectedIndex switch{0=>"blue",1=>"red",2=>"black",_=>null};
 static string VariantTag(MultiEncounter e)=>"["+e.VariantLabel+(e.IsBlack?$" · HP {e.hp}":"")+"]";
 // Real-control automation exercises declined/confirmed conversion and backup before mutation.
 internal void TestVariantEditing(MultiDocument d,PlacementGeometry g){
 data=d;loaded[g.Permit.key]=g;AddRoom(g);RefreshList(0);string before=JsonSerializer.Serialize(data);var backups=new List<string>();
 ConfirmVariantForTest=(_,_)=>false;
 BackupPathForTest=()=>{string p=Path.Combine(Path.GetTempPath(),"cp20-ui-"+Guid.NewGuid().ToString("N")+".json");backups.Add(p);return p;};
 try{
  MultiConfig.C(variantBox.SelectedIndex==3&&!hpBox.Enabled,"LEGACY_UI_DISTINCT");
  RefreshList(0);MultiConfig.C(JsonSerializer.Serialize(data)==before,"OPEN_SELECT_NO_CONVERSION");
  variantBox.SelectedIndex=0;MultiConfig.C(JsonSerializer.Serialize(data)==before&&backups.Count==0,"DECLINED_NO_FILE_OR_MODEL_CHANGE");
  ConfirmVariantForTest=(_,_)=>true;var first=data.encounters[0];variantBox.SelectedIndex=0;var changed=data.encounters[0];
  MultiConfig.C(changed.id==first.id&&changed.x==first.x&&changed.y==first.y&&changed.z==first.z&&changed.yaw_s16==first.yaw_s16&&!changed.enabled&&!changed.qa_confirmed&&changed.enemy=="E_OC","CONVERSION_FIELDS_QA");
  MultiConfig.C(backups.Count==1&&JsonSerializer.Serialize(MultiConfig.Decode(File.ReadAllBytes(backups[0])))==before,"UI_BACKUP_BEFORE_CONVERSION");
  MultiConfig.C(variantBox.SelectedIndex==0&&!hpBox.Enabled&&list.Items[0]!.ToString()!.Contains("Blue auténtico"),"BLUE_UI_LABEL");
  variantBox.SelectedIndex=1;MultiConfig.C(data.encounters[0].bokoblin_variant=="red"&&!hpBox.Enabled,"RED_UI_LABEL");
  variantBox.SelectedIndex=2;MultiConfig.C(data.encounters[0].IsBlack&&hpBox.Enabled,"BLACK_HP_UI");
  hpBox.Value=200;MultiConfig.C(data.encounters[0].hp==200,"BLACK_HP_EDIT");
 }finally{foreach(string p in backups)if(File.Exists(p))File.Delete(p);ConfirmVariantForTest=null;BackupPathForTest=null;}
 }
 internal void TestNewMixed(PlacementGeometry g){
 data=MultiDocument.Empty();loaded[g.Permit.key]=g;AddRoom(g);SelectRoom(g.Permit.key);RefreshList();layer.SelectedIndex=6;
 for(int i=0;i<3;i++){variantBox.SelectedIndex=i;Add(1000+i*400,1000);}
 MultiConfig.C(data.encounters.Select(e=>e.bokoblin_variant).SequenceEqual(new[]{"blue","red","black"})&&data.encounters.All(e=>!e.enabled&&!e.qa_confirmed&&e.yaw_s16==(int)yaw.Value),"NEW_MIXED_DISABLED_EXPLICIT");
 foreach(var e in data.encounters)g.Check(e);MultiConfig.Validate(data);
 }
 internal void TestMarkerRendering(string output){
 PerformLayout();Frame();using var bitmap=new Bitmap(canvas.Width,canvas.Height);canvas.DrawToBitmap(bitmap,new Rectangle(Point.Empty,canvas.Size));
 foreach(var item in data.encounters){var p=view.Screen(item.x,item.z,canvas.Size);var color=item.IsBlack?Color.Black:item.IsLegacy?Color.Teal:item.IsBlue?Color.DeepSkyBlue:Color.Red;MultiConfig.C(bitmap.GetPixel((int)p.X,(int)p.Y).ToArgb()==color.ToArgb(),"VARIANT_MARKER_RENDER_COLOR");}
 bitmap.Save(Path.Combine(output,"cp20-synthetic-map-markers.png"));
 }
 internal void TestRoomSelection(MultiDocument d,PlacementGeometry first,PlacementGeometry second){
 data=d;loaded[first.Permit.key]=first;loaded[second.Permit.key]=second;
 foreach(var g in loaded.Values)AddRoom(g);
 RefreshList(0);string before=JsonSerializer.Serialize(data);
 SelectRoom(second.Permit.key);layer.SelectedIndex=2;yaw.Value=123;
 MultiConfig.C(JsonSerializer.Serialize(data)==before&&list.SelectedIndex==-1,"ROOM_SELECTION_MUTATED_MARKER");
 SelectRoom(first.Permit.key);SelectRoom(second.Permit.key);SelectRoom(first.Permit.key);
 MultiConfig.C(JsonSerializer.Serialize(data)==before&&ReferenceEquals(current,first),"ROOM_SELECTION_ROUNDTRIP_MUTATED");
 }
 // CP10.1: two choices with an IDENTICAL visible label must still select by key, never by text.
 internal void TestLabelCollision(PlacementGeometry g1,PlacementGeometry g2){
 loaded[g1.Permit.key]=g1;loaded[g2.Permit.key]=g2;rooms.Items.Clear();
 rooms.Items.Add(new RoomChoice(g1.Permit.key,"SAME LABEL"));rooms.Items.Add(new RoomChoice(g2.Permit.key,"SAME LABEL"));
 SelectRoom(g2.Permit.key);MultiConfig.C(ReferenceEquals(current,g2),"LABEL_COLLISION_SECOND");
 SelectRoom(g1.Permit.key);MultiConfig.C(ReferenceEquals(current,g1),"LABEL_COLLISION_FIRST");
 }
 void Run(Action a){try{a();}catch(Exception e){status.Text="Operación rechazada: "+e.Message;SelectItem();}}
 bool Discard()=>!dirty||MessageBox.Show(this,"¿Descartar cambios sin guardar?","Borrador",MessageBoxButtons.YesNo)==DialogResult.Yes;
 public void Accept(ResourceInfo r,Mesh mesh,byte[] raw){
 var g=new PlacementGeometry(r,mesh,raw);
 var existing=data.resources.SingleOrDefault(x=>x.key==g.Permit.key);
 if(existing!=null)MultiConfig.C(Same(existing,g.Permit),"RESOURCE_PROVENANCE_CHANGED");
 loaded[g.Permit.key]=g;AddRoom(g);SelectRoom(g.Permit.key);BringToFront();
 status.Text=$"{NameResolver.Resolve(r.Stage,r.Room,r.Variant).Full}: ruta + PLC room + DZR presentes, geometría reconstruida. El sufijo ARC NO es capa. Elija capa explícita; nueva ubicación desactivada.";
 }
 public void ObserveCatalog(GameCatalog catalog){
  foreach(var r in Coverage.Rooms(catalog)){string key=Coverage.Key(r);observed[key]=r;if(!rooms.Items.Cast<RoomChoice>().Any(c=>c.Key==key))rooms.Items.Add(new RoomChoice(key,NameResolver.Resolve(r.Stage,r.Room,r.Variant).Full+" · "+r.Status));}
  status.Text=$"F_SP121: {observed.Count} recursos de sala observados; STG separado. Seleccione cualquiera; activación solo con suelo estricto.";
 }
 void PrepareBlocked(ResourceInfo r){
  using var form=new Form{Text=$"Intención desactivada {r.Stage}/R{r.Room:00}_{r.Variant}",Width=680,Height=380};
  var text=new TextBox{Dock=DockStyle.Fill,Multiline=true,MaxLength=512};var why=new Label{Dock=DockStyle.Top,Height=140,Text=Coverage.Reason(r)+"\nSin coordenadas inventadas. Esta intención NO es encounters.json y NO instala actores."};
  var save=new Button{Dock=DockStyle.Bottom,Height=40,Text="Guardar intención bloqueada…"};form.Controls.Add(text);form.Controls.Add(why);form.Controls.Add(save);
  save.Click+=(_,_)=>{using var d=new SaveFileDialog{Filter="Preparación bloqueada|*.preparation.json",FileName=$"F_SP121-R{r.Room:00}_{r.Variant}.preparation.json"};if(d.ShowDialog(form)!=DialogResult.OK)return;Run(()=>{Coverage.SavePreparation(d.FileName,r,text.Text);form.Close();});};form.ShowDialog(this);
 }
 internal int ObservedCount=>observed.Count;
 static bool Same(ResourcePermit a,ResourcePermit b)=>JsonSerializer.Serialize(a)==JsonSerializer.Serialize(b);
 void CheckResource(MultiEncounter e){var r=data.resources.Single(x=>x.key==e.resource);MultiConfig.C(loaded.TryGetValue(e.resource,out var g)&&Same(r,g.Permit),"CARGAR_RECURSO_ORIGINAL_EN_CATALOGO");g!.Check(e);}
 void ExportCheck(){MultiConfig.Validate(data);foreach(var e in data.encounters)CheckResource(e);}
 void Add(float x,float z){
 var g=current!;float y=g.Place(x,z);MultiConfig.C(data.encounters.Count<32,"CAPACIDAD_32");string id;int n=1;do{id="bokoblin-"+(n++).ToString("000");}while(data.encounters.Any(e=>e.id==id));
 var resources=new List<ResourcePermit>(data.resources);if(!resources.Any(r=>r.key==g.Permit.key))resources.Add(g.Permit);
 MultiConfig.C(SelectedVariant!=null,"Seleccione Blue, Red o Black explícito para un punto nuevo");
 var items=new List<MultiEncounter>(data.encounters){new(id,false,SelectedVariant=="blue"?"E_OC":"E_OC2",g.Permit.key,"F_SP121",g.Permit.room,layer.SelectedIndex,x,y,z,(int)yaw.Value,510,0xFFFF0100u,false,"QA_CANDIDATE",SelectedVariant,SelectedVariant=="black"?(int)hpBox.Value:null)};
 var draft=data with{resources=resources,encounters=items};MultiConfig.Validate(draft);data=draft;Changed();
 }
 void Replace(int i,MultiEncounter e){var items=new List<MultiEncounter>(data.encounters);items[i]=e;var draft=data with{encounters=items};MultiConfig.Validate(draft);data=draft;Changed(i);}
 void Changed(int selected=-1){dirty=true;RefreshList(selected);status.Text="Borrador modificado. Ningún actor creado. Exporte e instale con Dusklight cerrado.";Text="CP20 multi-room · cambios sin guardar";}
 void RefreshList(int selected=-1){updating=true;list.Items.Clear();foreach(var e in data.encounters)list.Items.Add($"{e.id} {VariantTag(e)} · {PlaceOf(e)} · Room {e.room:00} / Layer {e.layer} · {(e.enabled?"QA ON":"OFF")}");list.SelectedIndex=selected>=0&&selected<list.Items.Count?selected:-1;updating=false;SelectItem();canvas.Invalidate();}
 void SelectItem(){if(updating)return;updating=true;try{int i=list.SelectedIndex;enabled.Enabled=i>=0;if(i<0){enabled.Checked=false;detail.Text="Seleccione un marcador. Los recursos de otras salas no se mezclan.";return;}var e=data.encounters[i];SelectRoom(e.resource);enabled.Checked=e.enabled;yaw.Value=e.yaw_s16;layer.SelectedIndex=e.layer;variantBox.SelectedIndex=e.IsLegacy?3:e.IsBlue?0:e.IsBlack?2:1;hpBox.Enabled=e.IsBlack;hpBox.Value=e.hp is >=MultiConfig.HpMin and <=MultiConfig.HpMax?e.hp.Value:MultiConfig.HpDefaultBlack;detail.Text=$"{e.id} {VariantTag(e)}\n{e.VariantLabel}\n{PlaceOf(e)} · Room {e.room:00}\n{e.stage} / room {e.room} / layer {e.layer}\nX={e.x:G9} Y={e.y:G9} Z={e.z:G9}\n{e.layer_state}\nRegistro futuro, no aparición observada.";}finally{updating=false;}}
 int Hit(Point p){if(current==null)return -1;double best=14;int hit=-1;for(int i=0;i<data.encounters.Count;i++){var e=data.encounters[i];if(e.resource!=current.Permit.key)continue;var s=view.Screen(e.x,e.z,canvas.Size);double d=Math.Sqrt(Math.Pow(p.X-s.X,2)+Math.Pow(p.Y-s.Y,2));if(d<best){best=d;hit=i;}}return hit;}
 void Frame(){if(current==null)return;var m=current.Mesh;view.CenterX=(m.MinX+m.MaxX)/2;view.CenterZ=(m.MinZ+m.MaxZ)/2;view.Scale=Math.Max(.00001,Math.Min((canvas.Width-40)/Math.Max(1,m.MaxX-m.MinX),(canvas.Height-40)/Math.Max(1,m.MaxZ-m.MinZ)));canvas.Invalidate();}
 void Paint(object? sender,PaintEventArgs e){if(current!=null&&raster!=null){var m=current.Mesh;var a=view.Screen(m.MinX,m.MaxZ,canvas.Size);var b=view.Screen(m.MaxX,m.MinZ,canvas.Size);e.Graphics.DrawImage(raster,new RectangleF(a.X,a.Y,Math.Max(.01f,b.X-a.X),Math.Max(.01f,b.Y-a.Y)));foreach(var item in data.encounters.Where(x=>x.resource==current.Permit.key)){var p=view.Screen(item.x,item.z,canvas.Size);e.Graphics.FillEllipse(item.enabled?Brushes.Orange:Brushes.LightGray,p.X-6,p.Y-6,12,12);e.Graphics.FillEllipse(item.IsBlack?Brushes.Black:item.IsLegacy?Brushes.Teal:item.IsBlue?Brushes.DeepSkyBlue:Brushes.Red,p.X-4,p.Y-4,8,8);e.Graphics.DrawString(item.id+" L"+item.layer,Font,Brushes.White,p.X+8,p.Y);}}e.Graphics.DrawString("N (+Z) ↑ · X → · geometría local; QA, no atlas global",Font,Brushes.White,10,10);}
 void Open(){if(!Discard())return;using var d=new OpenFileDialog{Filter="JSON / respaldo|*.json;*.backup-*|Todos|*.*"};if(d.ShowDialog(this)!=DialogResult.OK)return;var bytes=MultiConfig.Read(d.FileName);using var j=JsonDocument.Parse(bytes);if(j.RootElement.GetProperty("schema_version").GetInt32()==1){status.Text="Schema1 intacto: abrir en editor de compatibilidad CP08, o usar Copiar legacy (con mapa R03 cargado). No se migró ni escribió.";return;}data=MultiConfig.Decode(bytes);path=d.FileName;expected=Disc.Hash(bytes);dirty=false;RefreshList();status.Text="Abierto. Cargue sus recursos desde CISO antes de activar/exportar. Archivo sin modificar.";}
 public static MultiDocument ConvertLegacy(Document old,PlacementGeometry g){MultiConfig.C(g.Permit.arc_sha256==Geometry.Hash,"LEGACY_HASH");Config.Validate(old);var d=MultiDocument.Empty();d.resources.Add(g.Permit);foreach(var e in old.encounters){var n=new MultiEncounter(e.id,e.enabled,e.enemy,g.Permit.key,e.stage,e.room,e.layer,e.x,e.y,e.z,e.yaw_s16,510,0xFFFF0100u,true,"CP08_OBSERVED");g.Check(n);d.encounters.Add(n);}MultiConfig.Validate(d);return d;}
 void ImportLegacy(){if(!Discard())return;MultiConfig.C(loaded.TryGetValue("r03-00",out var g),"Cargue primero F_SP121/R03_00.arc desde catálogo");using var d=new OpenFileDialog{Filter="CP08 schema1|*.json"};if(d.ShowDialog(this)!=DialogResult.OK)return;if(MessageBox.Show(this,"Crear una COPIA schema2 sin modificar el original. ¿Continuar?","Migración explícita",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;data=ConvertLegacy(Config.Decode(Config.Read(d.FileName)),g!);path=expected=null;Changed();status.Text="Copia en memoria; posiciones legacy exactas. Guarde primero un archivo de prueba distinto.";}
 void Save(){VariantEditing.RequireGameClosed();ExportCheck();using var d=new SaveFileDialog{Filter="CP20 schema2|*.json",FileName=path==null?"encounters.cp10-test.json":Path.GetFileName(path)};if(d.ShowDialog(this)!=DialogResult.OK)return;string? hash=d.FileName==path?expected:MultiConfig.HashFile(d.FileName);MultiConfig.Save(d.FileName,data,hash);path=d.FileName;expected=MultiConfig.HashFile(path);dirty=false;Text="CP20 multi-room · guardado";status.Text="Prueba guardada y releída. No instalada automáticamente; no hay hot reload.";}
 void Install(){VariantEditing.RequireGameClosed();ExportCheck();using(var mods=new FolderBrowserDialog{Description="Carpeta mods activa: verificar una sola Expansion 0.16.3 y un solo Provider 0.14.0"}){if(mods.ShowDialog(this)!=DialogResult.OK)return;VariantEditing.RequirePair(mods.SelectedPath);}using var d=new FolderBrowserDialog{Description="Carpeta data_dir de dev.bokoblin.expansion",SelectedPath=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),"TwilitRealm","Dusklight","mod_data","dev.bokoblin.expansion")};if(d.ShowDialog(this)!=DialogResult.OK)return;string target=Path.Combine(d.SelectedPath,"encounters.json");string? hash=MultiConfig.HashFile(target);
 if(MessageBox.Show(this,"Confirme: Dusklight está completamente CERRADO, existe respaldo de saves y solo la pareja CP20 Provider 0.14.0 + Expansion 0.16.3 está instalada. Se respaldará y reemplazará encounters.json. ¿Continuar?","Instalación explícita",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;
 MultiConfig.Save(target,data,hash);status.Text="encounters.json verificado; respaldo exclusivo conservado si existía. Aplicación al próximo arranque frío. placements.json no se ha tocado.";}
}
