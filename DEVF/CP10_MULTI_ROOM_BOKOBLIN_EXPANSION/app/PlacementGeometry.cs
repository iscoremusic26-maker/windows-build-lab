namespace DevfMap;
public sealed class PlacementGeometry {
 public ResourcePermit Permit{get;}public Mesh Mesh{get;}
 readonly Geometry? legacy;
 public PlacementGeometry(ResourceInfo r,Mesh mesh,byte[] raw){
 MultiConfig.C(r.Stage=="F_SP121"&&r.Room is >=1 and <=5&&r.Variant=="00"&&r.Path.EndsWith($"Stage/F_SP121/R{r.Room:00}_00.arc",StringComparison.Ordinal)&&mesh.PlcRooms.Contains(r.Room!.Value),"RESOURCE_NOT_ELIGIBLE");
 MultiConfig.C(Disc.Hash(raw)==r.Sha256,"RESOURCE_HASH_CHANGED");
 var members=Archive.Read(raw);MultiConfig.C(members.Keys.Any(k=>k.EndsWith(".dzr",StringComparison.OrdinalIgnoreCase)),"DZR_MISSING");
 if(r.Stage=="F_SP121"&&r.Room==3&&r.Variant=="00"&&Disc.Hash(raw)==Geometry.Hash)legacy=Geometry.LoadBytes(raw);
 Mesh=mesh;Permit=new($"r{r.Room:00}-00","F_SP121",r.Room.Value,"00",$"Stage/F_SP121/R{r.Room:00}_00.arc",Disc.Hash(raw),mesh.KclHash,mesh.PlcHash,r.Room.Value,new[]{mesh.MinX,mesh.MaxX,mesh.MinY,mesh.MaxY,mesh.MinZ,mesh.MaxZ},"STATIC_QA_NOT_RUNTIME_VERIFIED");
 var d=MultiDocument.Empty();d.resources.Add(Permit);MultiConfig.Validate(d);
 }
 internal PlacementGeometry(ResourcePermit p,Mesh m){Permit=p;Mesh=m;}
 public float Place(float x,float z){
 var m=Mesh;MultiConfig.C(float.IsFinite(x)&&float.IsFinite(z)&&x>=m.MinX+100&&x<=m.MaxX-100&&z>=m.MinZ+100&&z<=m.MaxZ-100,"BOUNDS_MARGIN_100");
 double y=m.Ground(x,z,Permit.room),lo=y,hi=y;
 for(int dx=-100;dx<=100;dx+=50)for(int dz=-100;dz<=100;dz+=50){double h=m.Ground(x+dx,z+dz,Permit.room);lo=Math.Min(lo,h);hi=Math.Max(hi,h);}MultiConfig.C(hi-lo<=50,"FOOTPRINT_SLOPE");
 foreach(var t in m.Triangles.Where(t=>t.N.Y<.5&&(t.C0&(1u<<14))==0)){
 if(Math.Max(t.P.Y,Math.Max(t.Q.Y,t.R.Y))<y+5||Math.Min(t.P.Y,Math.Min(t.Q.Y,t.R.Y))>y+250)continue;
 double Distance(V a,V b){double vx=b.X-a.X,vz=b.Z-a.Z,l=vx*vx+vz*vz,u=l==0?0:Math.Clamp(((x-a.X)*vx+(z-a.Z)*vz)/l,0,1);return Math.Sqrt(Math.Pow(x-a.X-u*vx,2)+Math.Pow(z-a.Z-u*vz,2));}
 MultiConfig.C(Math.Min(Distance(t.P,t.Q),Math.Min(Distance(t.Q,t.R),Distance(t.R,t.P)))>=200,"WALL_CLEARANCE_200");
 }
 return (float)(y+5);
 }
 public void Check(MultiEncounter e){
 MultiConfig.C(e.resource==Permit.key&&e.room==Permit.room,"RESOURCE_CONTEXT");
 if(e.layer_state=="CP08_OBSERVED"){
  MultiConfig.C(legacy!=null&&Permit.stage=="F_SP121"&&Permit.room==3&&Permit.variant=="00"&&Permit.path=="Stage/F_SP121/R03_00.arc"&&Permit.arc_sha256==Geometry.Hash&&e.stage=="F_SP121"&&e.room==3&&e.layer==6,"LEGACY_CONTEXT_REQUIRED");
  var old=Document.Empty();old.encounters.Add(new(e.id,e.enabled,e.enemy,e.stage,e.room,e.layer,e.x,e.y,e.z,e.yaw_s16));
  Config.Validate(old);Config.GeometryCheck(old,legacy!);return;
 }
 MultiConfig.C(e.layer_state=="QA_CANDIDATE","PLACEMENT_STATE");
 MultiConfig.C(Math.Abs(e.y-Place((float)e.x,(float)e.z))<.02,"HEIGHT_CHANGED");
}
}
