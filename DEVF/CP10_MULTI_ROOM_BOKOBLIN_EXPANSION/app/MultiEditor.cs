using System.Text.Json;
namespace DevfMap;
public sealed class MultiEditor:Form{
 MultiDocument data=MultiDocument.Empty();readonly Dictionary<string,PlacementGeometry> loaded=new();
 readonly Canvas canvas=new(){Dock=DockStyle.Fill};readonly Projection view=new();readonly ListBox list=new(){Width=320,Height=260};
 readonly ComboBox rooms=new(){Width=180,DropDownStyle=ComboBoxStyle.DropDownList},tool=new(){Width=160,DropDownStyle=ComboBoxStyle.DropDownList},layer=new(){Width=300,DropDownStyle=ComboBoxStyle.DropDownList};
 readonly CheckBox enabled=new(){Text="Habilitar para QA en arranque frío",Width=310};
 readonly NumericUpDown yaw=new(){Minimum=-32768,Maximum=32767,Width=180};
 readonly Label status=new(){Dock=DockStyle.Bottom,Height=100,Padding=new Padding(8)},detail=new(){Width=315,Height=105};
 string? path,expected;bool dirty,updating,pan;Point prior;int moving=-1;Bitmap? raster;
 PlacementGeometry? current=>rooms.SelectedItem is string key&&loaded.TryGetValue(key,out var g)?g:null;
 public int ModelCount=>data.encounters.Count;
 public MultiEditor(){
 Text="CP10 multi-room · F_SP121 · TEST privado";Width=1350;Height=900;MinimumSize=new(1000,700);
 var top=new FlowLayoutPanel{Dock=DockStyle.Top,AutoSize=true,Padding=new Padding(5)};
 void Button(string name,Action action){var b=new Button{Text=name,AutoSize=true};b.Click+=(_,_)=>Run(action);top.Controls.Add(b);}
 Button("CISO / Catálogo",()=>new CatalogBrowser(raw=>{var e=new Editor();e.AcceptImported(raw);e.Show(this);},Accept).Show(this));
 Button("Abrir schema2",Open);Button("Copiar legacy schema1…",ImportLegacy);Button("Guardar prueba…",Save);
 Button("Instalar JSON… (juego cerrado)",Install);Button("Restaurar backup / Abrir",Open);
 Button("Descartar",()=>{if(!Discard())return;if(path!=null){data=MultiConfig.Decode(File.ReadAllBytes(path));expected=MultiConfig.HashFile(path);}else data=MultiDocument.Empty();dirty=false;RefreshList();});
 Button("Nuevo",()=>{if(!Discard())return;data=MultiDocument.Empty();path=expected=null;dirty=false;RefreshList();});
 Button("Encuadrar",Frame);top.Controls.Add(rooms);tool.Items.AddRange(new object[]{"Seleccionar / mover","Añadir","Desplazar"});tool.SelectedIndex=0;top.Controls.Add(tool);
 var side=new FlowLayoutPanel{Dock=DockStyle.Right,Width=340,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Padding=new Padding(8)};
 side.Controls.Add(new Label{Width=315,Height=60,Text="E_OC2 nativo · máximo 8 global\nSolo F_SP121 / salas 1–5 / variante00\nCapa candidata; gameplay NO VERIFICADO"});
 side.Controls.Add(list);for(int i=0;i<15;i++)layer.Items.Add("Capa "+i+" · candidata QA");side.Controls.Add(layer);side.Controls.Add(enabled);side.Controls.Add(new Label{Text="Yaw s16",AutoSize=true});side.Controls.Add(yaw);side.Controls.Add(detail);
 void Act(string n,Action a){var b=new Button{Text=n,Width=310};b.Click+=(_,_)=>Run(a);side.Controls.Add(b);}
 Act("Duplicar en un nuevo punto",()=>{MultiConfig.C(list.SelectedIndex>=0,"Seleccione un encuentro");tool.SelectedIndex=1;status.Text="Clic en otro suelo: nuevo ID, desactivado. No se duplica en la misma posición.";});
 Act("Eliminar",()=>{int i=list.SelectedIndex;if(i>=0&&MessageBox.Show(this,"¿Eliminar del borrador?","Confirmar",MessageBoxButtons.YesNo)==DialogResult.Yes){data.encounters.RemoveAt(i);Changed();}});
 side.Controls.Add(new Label{Width=315,Height=160,Text="Nuevos puntos: disabled por defecto.\nActivación QA explícita; no prueba capa activa.\nClic: añadir/mover. Derecha: desplazar.\nRueda: zoom. Ningún cambio en vivo.\nLos recursos solo se importan localmente.\nGuardar exige volver a cargar sus mapas."});
 Controls.Add(canvas);Controls.Add(side);Controls.Add(top);Controls.Add(status);
 rooms.SelectedIndexChanged+=(_,_)=>Run(()=>{raster?.Dispose();raster=current==null?null:CatalogBrowser.Rasterize(current.Mesh,current.Permit.room,CancellationToken.None);Frame();});
 list.SelectedIndexChanged+=(_,_)=>SelectItem();
 enabled.CheckedChanged+=(_,_)=>{if(updating||list.SelectedIndex<0)return;Run(()=>{int i=list.SelectedIndex;var e=data.encounters[i];bool on=enabled.Checked;if(on){CheckResource(e);if(e.layer_state!="CP08_OBSERVED"&&MessageBox.Show(this,$"Sala {e.room}, capa {e.layer}: NO VERIFICADO EN RUNTIME. ¿Habilitar solo para QA en arranque frío?","Autorización QA",MessageBoxButtons.YesNo)!=DialogResult.Yes){SelectItem();return;}}Replace(i,e with{enabled=on,qa_confirmed=on||e.qa_confirmed});});};
 layer.SelectedIndexChanged+=(_,_)=>{if(updating||list.SelectedIndex<0||layer.SelectedIndex<0)return;Run(()=>{int i=list.SelectedIndex;var e=data.encounters[i];if(e.layer!=layer.SelectedIndex)Replace(i,e with{layer=layer.SelectedIndex,enabled=false,qa_confirmed=false,layer_state="QA_CANDIDATE"});});};
 yaw.ValueChanged+=(_,_)=>{if(!updating&&list.SelectedIndex>=0)Run(()=>{int i=list.SelectedIndex;Replace(i,data.encounters[i] with{yaw_s16=(int)yaw.Value});});};
 canvas.Paint+=Paint;canvas.MouseEnter+=(_,_)=>canvas.Focus();
 canvas.MouseWheel+=(_,e)=>{var a=view.World(e.Location,canvas.Size);view.Scale=Math.Clamp(view.Scale*(e.Delta>0?1.2:1/1.2),.00001,10);var b=view.World(e.Location,canvas.Size);view.CenterX+=a.X-b.X;view.CenterZ+=a.Z-b.Z;canvas.Invalidate();};
 canvas.MouseDown+=(_,e)=>Run(()=>{if(e.Button==MouseButtons.Right||tool.SelectedIndex==2){pan=true;prior=e.Location;canvas.Capture=true;return;}if(e.Button!=MouseButtons.Left||current==null)return;
 if(tool.SelectedIndex==1){MultiConfig.C(layer.SelectedIndex>=0,"Seleccione explícitamente una capa candidata (0–14)");var p=view.World(e.Location,canvas.Size);Add(p.X,p.Z);return;}
 moving=Hit(e.Location);list.SelectedIndex=moving;if(moving>=0)canvas.Capture=true;});
 canvas.MouseMove+=(_,e)=>{if(pan){view.CenterX-=(e.X-prior.X)/view.Scale;view.CenterZ+=(e.Y-prior.Y)/view.Scale;prior=e.Location;canvas.Invalidate();}};
 canvas.MouseUp+=(_,e)=>{if(moving>=0&&current!=null){int i=moving;Run(()=>{var p=view.World(e.Location,canvas.Size);var old=data.encounters[i];var y=current.Place(p.X,p.Z);if(p.X!=(float)old.x||p.Z!=(float)old.z)Replace(i,old with{x=p.X,y=y,z=p.Z,enabled=false,qa_confirmed=false,layer_state="QA_CANDIDATE"});});}moving=-1;pan=false;canvas.Capture=false;};
 FormClosing+=(_,e)=>{if(!Discard())e.Cancel=true;};FormClosed+=(_,_)=>raster?.Dispose();
 status.Text="Importe el CISO probado y seleccione un recurso F_SP121. Salas 1,2,4,5: candidatas; ninguna capa nueva está observada en runtime.";
 }
 void Run(Action a){try{a();}catch(Exception e){status.Text="Operación rechazada: "+e.Message;SelectItem();}}
 bool Discard()=>!dirty||MessageBox.Show(this,"¿Descartar cambios sin guardar?","Borrador",MessageBoxButtons.YesNo)==DialogResult.Yes;
 public void Accept(ResourceInfo r,Mesh mesh,byte[] raw){
 var g=new PlacementGeometry(r,mesh,raw);
 var existing=data.resources.SingleOrDefault(x=>x.key==g.Permit.key);
 if(existing!=null)MultiConfig.C(Same(existing,g.Permit),"RESOURCE_PROVENANCE_CHANGED");
 loaded[g.Permit.key]=g;if(!rooms.Items.Contains(g.Permit.key))rooms.Items.Add(g.Permit.key);rooms.SelectedItem=g.Permit.key;BringToFront();
 status.Text=$"Sala {r.Room}: ruta + PLC room + DZR presentes, geometría reconstruida. El sufijo00 NO es capa. Elija capa explícita; nueva ubicación desactivada.";
 }
 static bool Same(ResourcePermit a,ResourcePermit b)=>JsonSerializer.Serialize(a)==JsonSerializer.Serialize(b);
 void CheckResource(MultiEncounter e){var r=data.resources.Single(x=>x.key==e.resource);MultiConfig.C(loaded.TryGetValue(e.resource,out var g)&&Same(r,g.Permit),"CARGAR_RECURSO_ORIGINAL_EN_CATALOGO");g!.Check(e);}
 void ExportCheck(){MultiConfig.Validate(data);foreach(var e in data.encounters)CheckResource(e);}
 void Add(float x,float z){
 var g=current!;float y=g.Place(x,z);MultiConfig.C(data.encounters.Count<8,"CAPACIDAD_8");string id;int n=1;do{id="bokoblin-"+(n++).ToString("000");}while(data.encounters.Any(e=>e.id==id));
 var resources=new List<ResourcePermit>(data.resources);if(!resources.Any(r=>r.key==g.Permit.key))resources.Add(g.Permit);
 var items=new List<MultiEncounter>(data.encounters){new(id,false,"E_OC2",g.Permit.key,"F_SP121",g.Permit.room,layer.SelectedIndex,x,y,z,(int)yaw.Value,510,0xFFFF0100u,false,"QA_CANDIDATE")};
 var draft=data with{resources=resources,encounters=items};MultiConfig.Validate(draft);data=draft;Changed(items.Count-1);
 }
 void Replace(int i,MultiEncounter e){var items=new List<MultiEncounter>(data.encounters);items[i]=e;var draft=data with{encounters=items};MultiConfig.Validate(draft);data=draft;Changed(i);}
 void Changed(int selected=-1){dirty=true;RefreshList(selected);status.Text="Borrador modificado. Ningún actor creado. Exporte e instale con Dusklight cerrado.";Text="CP10 multi-room · cambios sin guardar";}
 void RefreshList(int selected=-1){updating=true;list.Items.Clear();foreach(var e in data.encounters)list.Items.Add($"{e.id} · R{e.room}/L{e.layer} · {(e.enabled?"QA ON":"OFF")}");list.SelectedIndex=selected>=0&&selected<list.Items.Count?selected:-1;updating=false;SelectItem();canvas.Invalidate();}
 void SelectItem(){if(updating)return;updating=true;try{int i=list.SelectedIndex;enabled.Enabled=i>=0;if(i<0){enabled.Checked=false;detail.Text="Seleccione un marcador. Los recursos de otras salas no se mezclan.";return;}var e=data.encounters[i];if(rooms.Items.Contains(e.resource))rooms.SelectedItem=e.resource;enabled.Checked=e.enabled;yaw.Value=e.yaw_s16;layer.SelectedIndex=e.layer;detail.Text=$"{e.id} · {e.stage} / {e.room} / {e.layer}\nX={e.x:G9} Y={e.y:G9} Z={e.z:G9}\n{e.layer_state}\nRegistro futuro, no aparición observada.";}finally{updating=false;}}
 int Hit(Point p){if(current==null)return -1;double best=14;int hit=-1;for(int i=0;i<data.encounters.Count;i++){var e=data.encounters[i];if(e.resource!=current.Permit.key)continue;var s=view.Screen(e.x,e.z,canvas.Size);double d=Math.Sqrt(Math.Pow(p.X-s.X,2)+Math.Pow(p.Y-s.Y,2));if(d<best){best=d;hit=i;}}return hit;}
 void Frame(){if(current==null)return;var m=current.Mesh;view.CenterX=(m.MinX+m.MaxX)/2;view.CenterZ=(m.MinZ+m.MaxZ)/2;view.Scale=Math.Max(.00001,Math.Min((canvas.Width-40)/Math.Max(1,m.MaxX-m.MinX),(canvas.Height-40)/Math.Max(1,m.MaxZ-m.MinZ)));canvas.Invalidate();}
 void Paint(object? sender,PaintEventArgs e){if(current!=null&&raster!=null){var m=current.Mesh;var a=view.Screen(m.MinX,m.MaxZ,canvas.Size);var b=view.Screen(m.MaxX,m.MinZ,canvas.Size);e.Graphics.DrawImage(raster,new RectangleF(a.X,a.Y,Math.Max(.01f,b.X-a.X),Math.Max(.01f,b.Y-a.Y)));foreach(var item in data.encounters.Where(x=>x.resource==current.Permit.key)){var p=view.Screen(item.x,item.z,canvas.Size);e.Graphics.FillEllipse(item.enabled?Brushes.Orange:Brushes.LightGray,p.X-6,p.Y-6,12,12);e.Graphics.DrawString(item.id+" L"+item.layer,Font,Brushes.White,p.X+8,p.Y);}}e.Graphics.DrawString("N (+Z) ↑ · X → · geometría local; QA, no atlas global",Font,Brushes.White,10,10);}
 void Open(){if(!Discard())return;using var d=new OpenFileDialog{Filter="JSON / respaldo|*.json;*.backup-*|Todos|*.*"};if(d.ShowDialog(this)!=DialogResult.OK)return;var bytes=File.ReadAllBytes(d.FileName);using var j=JsonDocument.Parse(bytes);if(j.RootElement.GetProperty("schema_version").GetInt32()==1){status.Text="Schema1 intacto: abrir en editor de compatibilidad CP08, o usar Copiar legacy (con mapa R03 cargado). No se migró ni escribió.";return;}data=MultiConfig.Decode(bytes);path=d.FileName;expected=Disc.Hash(bytes);dirty=false;RefreshList();status.Text="Abierto. Cargue sus recursos desde CISO antes de activar/exportar. Archivo sin modificar.";}
 public static MultiDocument ConvertLegacy(Document old,PlacementGeometry g){MultiConfig.C(g.Permit.arc_sha256==Geometry.Hash,"LEGACY_HASH");Config.Validate(old);var d=MultiDocument.Empty();d.resources.Add(g.Permit);foreach(var e in old.encounters){var n=new MultiEncounter(e.id,e.enabled,e.enemy,g.Permit.key,e.stage,e.room,e.layer,e.x,e.y,e.z,e.yaw_s16,510,0xFFFF0100u,true,"CP08_OBSERVED");g.Check(n);d.encounters.Add(n);}MultiConfig.Validate(d);return d;}
 void ImportLegacy(){if(!Discard())return;MultiConfig.C(loaded.TryGetValue("r03-00",out var g),"Cargue primero F_SP121/R03_00.arc desde catálogo");using var d=new OpenFileDialog{Filter="CP08 schema1|*.json"};if(d.ShowDialog(this)!=DialogResult.OK)return;if(MessageBox.Show(this,"Crear una COPIA schema2 sin modificar el original. ¿Continuar?","Migración explícita",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;data=ConvertLegacy(Config.Decode(Config.Read(d.FileName)),g!);path=expected=null;Changed();status.Text="Copia en memoria; posiciones legacy exactas. Guarde primero un archivo de prueba distinto.";}
 void Save(){ExportCheck();using var d=new SaveFileDialog{Filter="CP10 schema2|*.json",FileName=path==null?"encounters.cp10-test.json":Path.GetFileName(path)};if(d.ShowDialog(this)!=DialogResult.OK)return;string? hash=d.FileName==path?expected:MultiConfig.HashFile(d.FileName);MultiConfig.Save(d.FileName,data,hash);path=d.FileName;expected=MultiConfig.HashFile(path);dirty=false;Text="CP10 multi-room · guardado";status.Text="Prueba guardada y releída. No instalada automáticamente; no hay hot reload.";}
 void Install(){ExportCheck();using var d=new FolderBrowserDialog{Description="Carpeta data_dir de dev.bokoblin.expansion",SelectedPath=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),"TwilitRealm","Dusklight","mod_data","dev.bokoblin.expansion")};if(d.ShowDialog(this)!=DialogResult.OK)return;string target=Path.Combine(d.SelectedPath,"encounters.json");string? hash=MultiConfig.HashFile(target);
 if(MessageBox.Show(this,"Confirme: Dusklight está completamente CERRADO, existe respaldo de saves y solo la pareja CP10 será instalada. Se respaldará y reemplazará encounters.json. ¿Continuar?","Instalación explícita",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;
 MultiConfig.Save(target,data,hash);status.Text="encounters.json verificado; respaldo exclusivo conservado si existía. Aplicación al próximo arranque frío. placements.json no se ha tocado.";}
}
