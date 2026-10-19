namespace DevfMap;
public sealed class CatalogBrowser:Form {
 readonly TreeView tree=new(){Dock=DockStyle.Left,Width=430,ShowNodeToolTips=true};readonly Canvas canvas=new(){Dock=DockStyle.Fill};
 readonly Label info=new(){Dock=DockStyle.Bottom,Height=150,Padding=new Padding(8)};
 readonly Button import=new(){Text="Importar juego CISO / ISO",AutoSize=true},last=new(){Text="Abrir imagen anterior",AutoSize=true},cancel=new(){Text="Cancelar",Enabled=false},compat=new(){Text="Editar en modo CP08",AutoSize=true,Enabled=false};
 readonly Projection view=new();CancellationTokenSource? operation;GameCatalog? catalog;string? image;Mesh? mesh;byte[]? raw;ResourceInfo? selected;Bitmap? raster;bool closing,drag;Point prior;
 readonly Action<GameCatalog>? catalogReady;readonly Action<ResourceInfo>? prepareBlocked;
 readonly Action<byte[]> openCompat;readonly Action<ResourceInfo,Mesh,byte[]>? openMulti;
 readonly Button multi=new(){Text="Seleccionar / preparar en CP13",AutoSize=true,Enabled=false};
 public CatalogBrowser(Action<byte[]> open,Action<ResourceInfo,Mesh,byte[]>? multiOpen=null,Action<GameCatalog>? observed=null,Action<ResourceInfo>? blocked=null){
  catalogReady=observed;prepareBlocked=blocked;openCompat=open;openMulti=multiOpen;Text="DEVF CP13 · catálogo local (importador CP09) · nombres en inglés (alias) · todas las entradas F_SP121 visibles";Width=1250;Height=850;MinimumSize=new(900,600);
  var report=new Button{Text="Exportar reporte F_SP121 (sin recursos)",AutoSize=true};
  var bar=new FlowLayoutPanel{Dock=DockStyle.Top,AutoSize=true};bar.Controls.AddRange(new Control[]{import,last,cancel,compat,multi,report});
  report.Click+=async(_,_)=>{if(catalog==null||image==null)return;using var d=new SaveFileDialog{Filter="Metadata JSON|*.json",FileName="CP13-F_SP121-metadata.json"};if(d.ShowDialog(this)!=DialogResult.OK)return;string dest=d.FileName;await Work(async token=>{var rows=await Task.Run(()=>Coverage.Report(image,catalog,token),token);token.ThrowIfCancellationRequested();var b=System.Text.Json.JsonSerializer.SerializeToUtf8Bytes(rows,CatalogService.Options);File.WriteAllBytes(dest,b);if(!File.ReadAllBytes(dest).SequenceEqual(b))throw new IOException("REPORT_VERIFY");info.Text="Reporte local verificado; sin ruta CISO, geometría ni hash de disco. Puede compartir solo este JSON.";});};
  multi.Click+=(_,_)=>{try{if(selected!=null&&(mesh==null||raw==null)){prepareBlocked?.Invoke(selected);return;}if(selected!=null&&mesh!=null&&raw!=null&&openMulti!=null){openMulti(selected,mesh,raw);info.Text="Recurso enviado a CP10. Capa candidata: NO VERIFICADA EN RUNTIME.";}}catch(Exception e){info.Text="CP10 bloqueado: "+e.Message;}};
  var frame=new Button{Text="Encuadrar",AutoSize=true};bar.Controls.Add(frame);frame.Click+=(_,_)=>Frame();
  Controls.Add(canvas);Controls.Add(tree);Controls.Add(bar);Controls.Add(info);
  import.Click+=async(_,_)=>{using var d=new OpenFileDialog{Filter="GameCube USA|*.ciso;*.iso;*.gcm",Title="Copia legítima GZ2E01 · solo lectura"};if(d.ShowDialog(this)==DialogResult.OK)await Import(d.FileName);};
  last.Click+=async(_,_)=>{try{var path=CatalogService.LastImage();if(path==null||!File.Exists(path)){info.Text="No hay imagen anterior disponible; use Importar juego.";return;}await Import(path);}catch{info.Text="No se pudo abrir la referencia local. Seleccione la imagen."; }};
  cancel.Click+=(_,_)=>operation?.Cancel();tree.AfterSelect+=async(_,e)=>{if(e.Node?.Tag is ResourceInfo r)await Select(r);};
  compat.Click+=(_,_)=>{if(selected?.Cp08Compatible==true&&raw!=null){try{openCompat(raw);info.Text="Modo CP08 abierto. Solo el rectángulo histórico admite exportación."; }catch{info.Text="No se pudo validar el modo CP08.";}}};
  canvas.Paint+=PaintMap;canvas.MouseEnter+=(_,_)=>canvas.Focus();
  canvas.MouseWheel+=(_,e)=>{var before=view.World(e.Location,canvas.Size);view.Scale=Math.Clamp(view.Scale*(e.Delta>0?1.2:1/1.2),.00001,10);var after=view.World(e.Location,canvas.Size);view.CenterX+=before.X-after.X;view.CenterZ+=before.Z-after.Z;canvas.Invalidate();};
  canvas.MouseDown+=(_,e)=>{if(e.Button is MouseButtons.Left or MouseButtons.Right){drag=true;prior=e.Location;canvas.Capture=true;}};
  canvas.MouseMove+=(_,e)=>{if(!drag)return;view.CenterX-=(e.X-prior.X)/view.Scale;view.CenterZ+=(e.Y-prior.Y)/view.Scale;prior=e.Location;canvas.Invalidate();};
  canvas.MouseUp+=(_,_)=>{drag=false;canvas.Capture=false;};
  FormClosing+=(_,e)=>{if(operation!=null){closing=true;e.Cancel=true;operation.Cancel();}};
  FormClosed+=(_,_)=>raster?.Dispose();info.Text="Seleccione una imagen CISO sparse GameCube USA GZ2E01 revisión 0. No se extraen archivos al disco. Las salas nuevas no permiten crear encuentros.";
 }
 async Task Work(Func<CancellationToken,Task> work){
  if(operation!=null)return;operation=new();import.Enabled=last.Enabled=tree.Enabled=compat.Enabled=multi.Enabled=false;cancel.Enabled=true;
  try{await work(operation.Token);}catch(OperationCanceledException){info.Text="Cancelado. El catálogo anterior en disco permanece intacto.";}
  catch(InvalidDataException e){info.Text="Importación rechazada: "+e.Message;}
  catch{info.Text="Operación incompleta: archivo inaccesible, formato dañado o error de lectura. No se modificó la imagen.";}
  finally{operation.Dispose();operation=null;cancel.Enabled=false;import.Enabled=last.Enabled=tree.Enabled=true;compat.Enabled=selected?.Cp08Compatible==true&&raw!=null;multi.Enabled=openMulti!=null&&selected?.Stage=="F_SP121"&&selected?.Room is >=0 and <=63;if(closing)Close();}
 }
 async Task Import(string path){
  await Work(async token=>{
   var progress=new Progress<string>(s=>{if(!IsDisposed)info.Text=s;});
   var result=await Task.Run(()=>CatalogService.Import(path,CatalogService.LocalRoot,token,progress),token);
   token.ThrowIfCancellationRequested();image=path;catalog=result;mesh=null;raw=null;selected=null;raster?.Dispose();raster=null;
   tree.BeginUpdate();tree.Nodes.Clear();foreach(var group in result.Resources.GroupBy(r=>r.Stage).OrderBy(g=>g.Key)){
    var node=tree.Nodes.Add(NameResolver.StageText(group.Key));node.ToolTipText=NameResolver.StageTip(group.Key);foreach(var r in group.OrderBy(r=>r.Room).ThenBy(r=>r.Path)){var lab=NameResolver.Resolve(r.Stage,r.Room,r.Variant);var child=node.Nodes.Add($"{lab.Short} · variant {r.Variant} · {r.Status}");child.Tag=r;child.ToolTipText=lab.Full+"\n"+lab.Provenance+"\n"+r.Path;}
   }tree.EndUpdate();catalogReady?.Invoke(result);canvas.Invalidate();
   int rooms=result.Resources.Where(r=>r.Room.HasValue).Select(r=>(r.Stage,r.Room)).Distinct().Count();
   info.Text=$"{result.DiscId} rev {result.Revision} · {result.Resources.Select(r=>r.Stage).Distinct().Count()} stages · {rooms} salas · {result.Resources.Count} recursos · geometría {result.Resources.Count(r=>r.Status=="GEOMETRY_AVAILABLE")}.\nCoordenadas nativas por recurso. Layers de ejecución desconocidos; no se deducen del sufijo ARC. Salas nuevas: visualización; exportación CP10 pendiente.";
   try{CatalogService.Remember(path);}catch{info.Text+="\nNo se pudo recordar la ruta; podrá seleccionarla de nuevo.";}
  });
 }
 async Task Select(ResourceInfo r){
  if(operation!=null||catalog==null||image==null)return;
  multi.Enabled=false;selected=r;raw=null;mesh=null;raster?.Dispose();raster=null;compat.Enabled=false;canvas.Invalidate();
  if(r.Status!="GEOMETRY_AVAILABLE"){multi.Enabled=openMulti!=null&&r.Stage=="F_SP121"&&r.Room is >=0 and <=63;info.Text=NameResolver.Resolve(r.Stage,r.Room,r.Variant).Full+"\n"+r.Path+"\n"+r.Status+"\nRecurso conservado en catálogo; no se habilita exportación.";return;}
  await Work(async token=>{
   string path=image;var c=catalog;var loaded=await Task.Run(()=>CatalogService.Load(path,c!,r,token),token);
   var picture=await Task.Run(()=>Rasterize(loaded.Mesh,r.Room,token),token);
   if(token.IsCancellationRequested){picture.Dispose();token.ThrowIfCancellationRequested();}
   mesh=loaded.Mesh;raw=loaded.Raw;raster=picture;Frame();
   info.Text=$"{NameResolver.Resolve(r.Stage,r.Room,r.Variant).Full}\n{NameResolver.Resolve(r.Stage,r.Room,r.Variant).Provenance}\n{r.Path}\n{mesh.Triangles.Count} triángulos · X [{mesh.MinX:F2}, {mesh.MaxX:F2}] · Y [{mesh.MinY:F2}, {mesh.MaxY:F2}] · Z [{mesh.MinZ:F2}, {mesh.MaxZ:F2}]\nSala inferida por ruta={r.Room}; salas PLC={string.Join(",",r.PlcRooms)}; chunks de capa={string.Join(",",r.LayerChunks)} (no prueba capa activa).\n"+(r.Cp08Compatible?"Modo CP08 disponible solo en el rectángulo histórico F_SP121/3/6.":"SOLO VISUALIZACIÓN. Geometría no certifica suelo, navegación ni aparición. Capa y suelo requieren validación CP13; no runtime PASS.");
  });
 }
 public static Bitmap Rasterize(Mesh m,int? room,CancellationToken token){
  var bitmap=new Bitmap(1200,1200);try{using var g=Graphics.FromImage(bitmap);g.Clear(Color.FromArgb(24,30,38));
   double dx=Math.Max(1,m.MaxX-m.MinX),dz=Math.Max(1,m.MaxZ-m.MinZ);
   PointF P(V v)=>new((float)((v.X-m.MinX)/dx*1199),(float)((m.MaxZ-v.Z)/dz*1199));
   using var floor=new SolidBrush(Color.FromArgb(65,125,103));using var other=new SolidBrush(Color.FromArgb(90,83,74));
   foreach(var t in m.Triangles){token.ThrowIfCancellationRequested();g.FillPolygon(room.HasValue&&Mesh.Floor(t,room.Value)?floor:other,new[]{P(t.P),P(t.Q),P(t.R)});}return bitmap;
  }catch{bitmap.Dispose();throw;}
 }
 void Frame(){if(mesh==null)return;view.CenterX=(mesh.MinX+mesh.MaxX)/2;view.CenterZ=(mesh.MinZ+mesh.MaxZ)/2;view.Scale=Math.Max(.00001,Math.Min(Math.Max(1,canvas.Width-40)/Math.Max(1,mesh.MaxX-mesh.MinX),Math.Max(1,canvas.Height-40)/Math.Max(1,mesh.MaxZ-mesh.MinZ)));canvas.Invalidate();}
 void PaintMap(object? sender,PaintEventArgs e){
  if(mesh!=null&&raster!=null){var a=view.Screen(mesh.MinX,mesh.MaxZ,canvas.Size);var b=view.Screen(mesh.MaxX,mesh.MinZ,canvas.Size);e.Graphics.DrawImage(raster,new RectangleF(a.X,a.Y,Math.Max(.01f,b.X-a.X),Math.Max(.01f,b.Y-a.Y)));}
  e.Graphics.DrawString("N (+Z) ↑ · X → · coordenadas nativas · rueda zoom / arrastrar desplaza\nKCL técnico; color no certifica colocación segura. Sin exportación fuera del modo CP08.",Font,Brushes.White,10,10);
 }
}
