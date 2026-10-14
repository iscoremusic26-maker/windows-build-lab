import fs from "node:fs";
const helpers=(function(){
function base64(s){const chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",clean=s.replace(/\s/g,"");let out=[],acc=0,bits=0;for(const c of clean){if(c==="=")break;const v=chars.indexOf(c);if(v<0)throw Error("badbase64");acc=(acc<<6)|v;bits+=6;if(bits>=8){bits-=8;out.push((acc>>>bits)&255);}}return new Uint8Array(out)}
function hex(v){return(v>>>0).toString(16).padStart(8,"0")}
function padded(x){let a=new Uint8Array(Math.ceil((x.length+9)/64)*64);a.set(x);a[x.length]=128;new DataView(a.buffer).setUint32(a.length-4,x.length*8,false);return a}
function sha256(x){const k=[0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2],r=(n,c)=>(n>>>c)|(n<<(32-c));let h=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19],a=padded(x),d=new DataView(a.buffer);for(let off=0;off<a.length;off+=64){let w=new Uint32Array(64);for(let j=0;j<16;j++)w[j]=d.getUint32(off+j*4,false);for(let j=16;j<64;j++){let u=w[j-15],v=w[j-2];w[j]=(w[j-16]+(r(u,7)^r(u,18)^(u>>>3))+w[j-7]+(r(v,17)^r(v,19)^(v>>>10)))>>>0}let [A,B,C,D,E,F,G,H]=h;for(let j=0;j<64;j++){let t=(H+(r(E,6)^r(E,11)^r(E,25))+((E&F)^(~E&G))+k[j]+w[j])>>>0,u=((r(A,2)^r(A,13)^r(A,22))+((A&B)^(A&C)^(B&C)))>>>0;H=G;G=F;F=E;E=(D+t)>>>0;D=C;C=B;B=A;A=(t+u)>>>0}let v=[A,B,C,D,E,F,G,H];h=h.map((z,j)=>(z+v[j])>>>0)}return h.map(hex).join("")}
function sha1(x){const rol=(x,b)=>(x<<b)|(x>>>(32-b));let h=[0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0],a=padded(x),d=new DataView(a.buffer);for(let off=0;off<a.length;off+=64){let w=new Uint32Array(80);for(let i=0;i<16;i++)w[i]=d.getUint32(off+i*4,false);for(let i=16;i<80;i++)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);let[A,B,C,D,E]=h;for(let i=0;i<80;i++){let f,k;if(i<20){f=(B&C)|(~B&D);k=0x5a827999}else if(i<40){f=B^C^D;k=0x6ed9eba1}else if(i<60){f=(B&C)|(B&D)|(C&D);k=0x8f1bbcdc}else{f=B^C^D;k=0xca62c1d6}let t=(rol(A,5)+f+E+k+w[i])>>>0;E=D;D=C;C=rol(B,30);B=A;A=t}h=h.map((v,i)=>(v+[A,B,C,D,E][i])>>>0)}return h.map(hex).join("")}
function crc32(a){let c=0xffffffff;for(const n of a){c^=n;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0)}return hex(c^0xffffffff)}
function u16(a,o){if(o<0||o+2>a.length)throw Error("u16 bounds");return(a[o]<<8)|a[o+1]}
function u32(a,o){if(o<0||o+4>a.length)throw Error("u32 bounds");return((a[o]*0x1000000)+(a[o+1]<<16)+(a[o+2]<<8)+a[o+3])>>>0}
function str(a,o,n){if(o<0||o+n>a.length)throw Error("str bounds");return String.fromCharCode(...a.slice(o,o+n))}
function cstr(a,o){let e=o;while(e<a.length&&a[e])e++;if(e===a.length)throw Error("cstr bounds");return str(a,o,e-o)}
function yaz0(a){if(str(a,0,4)!=="Yaz0")throw Error("Yaz0 magic");let size=u32(a,4);if(size>8e6)throw Error("Yaz0 size");let out=new Uint8Array(size),i=16,j=0,mask=0,code=0;while(j<size){if(!mask){if(i>=a.length)throw Error("code bounds");code=a[i++];mask=128}if(code&mask){if(i>=a.length)throw Error("literal bounds");out[j++]=a[i++]}else{if(i+2>a.length)throw Error("match bounds");let x=a[i++],y=a[i++],dist=((x&15)<<8)|y,len=x>>>4;if(!len){if(i>=a.length)throw Error("matchlen bounds");len=a[i++]+18}else len+=2;let p=j-dist-1;if(p<0||j+len>size)throw Error("match output bounds");for(let n=0;n<len;n++)out[j++]=out[p++]}mask>>>=1}return{data:out,consumed:i,trailing:a.length-i}}
function rarc(a){if(str(a,0,4)!=="RARC"||u32(a,4)!==a.length||u32(a,8)!==32)throw Error("RARCheader");let db=u32(a,12)+32,ec=u32(a,40),eb=u32(a,44)+32,sb=u32(a,52)+32,nc=u32(a,32),nb=u32(a,36)+32;if(eb+ec*20>a.length||nb+nc*16>a.length)throw Error("RARC tables");let files=[];for(let k=0;k<ec;k++){let o=eb+k*20,id=u16(a,o),flags=u16(a,o+4),name=cstr(a,sb+u16(a,o+6)),off=u32(a,o+8),size=u32(a,o+12);if(flags&0x0200)continue;if(!(flags&0x0100)||db+off+size>a.length)throw Error("RARCentry");files.push({id,name,flags,bytes:size,offset:db+off})}return{files,node_count:nc,entry_count:ec}}
function sections(a){if(str(a,0,8)!=="J3D2bmd3"||u32(a,8)!==a.length)throw Error("BMDmagic/size");let n=u32(a,12),out={},offsets={},o=32;for(let i=0;i<n;i++){let t=str(a,o,4),sz=u32(a,o+4);if(sz<8||o+sz>a.length||out[t])throw Error("BMDblock");out[t]=a.slice(o,o+sz);offsets[t]=o;o+=sz}if(o!==a.length)throw Error("BMDend");return{blocks:out,offsets}}
return{base64,sha256,sha1,crc32,u16,u32,str,cstr,yaz0,rarc,sections};
})();

function parseAll(raw){
 const h=helpers, y=h.yaz0(raw), arc=h.rarc(y.data);
 const manifest=JSON.parse(fs.readFileSync(process.argv[3],"utf8"));
 if(raw.length!==33872||h.sha256(raw)!=="c87d7a3cb6087ff5bcb8be3abab64b2b1124fe90bba0118ab61e174349bba394"||manifest.bytes!==raw.length||manifest.sha256!==h.sha256(raw))throw Error("ARC integrity/manifest mismatch");
 const prefix=Buffer.from("blob "+raw.length+"\0");
 const blob=h.sha1(Buffer.concat([prefix,raw]));
 if(blob!=="cf76c37c3575041b6b39faf9baec571bb1d6749f"||manifest.git_blob_sha1!==blob)throw Error("ARC Git blob mismatch");
 if(y.data.length!==47296||arc.files.length!==2)throw Error("RARC structure changed");
 let models=[];
 for(const f of arc.files){
   const expected=f.id===3?["oc2.bmd",39520,"223369ea238319eee4587d324458c8057f380994f15b04ff264da458bd180995"]:f.id===4?["oc2_wepon.bmd",7456,"ab736133d76c4d84924e5304a7c0b3d553f0706aa8a3a2758ee2456c1e8eef06"]:null;
   let b=y.data.slice(f.offset,f.offset+f.bytes);
   if(!expected||f.name!==expected[0]||f.bytes!==expected[1]||h.sha256(b)!==expected[2])throw Error("Model index/name/hash mismatch");
   let blocks=h.sections(b).blocks, a=blocks.MAT3;
   const count=h.u16(a,8), init=h.u32(a,12), remap=h.u32(a,16), names=h.u32(a,20);
   const table=f=>h.u32(a,f), row=(f,k,n)=>{if(k===65535)return null;let o=table(f)+k*n;if(o<8||o+n>a.length)throw Error("MAT3 table range");return Array.from(a.slice(o,o+n));};
   let mats=[];
   for(let i=0;i<count;i++){
     const map=h.u16(a,remap+2*i),p=init+map*332; if(p+332>a.length)throw Error("MAT3 init range");
     const name=h.cstr(a,names+h.u16(a,names+6+4*i)), idx=(o,n)=>Array.from({length:n},(_,j)=>h.u16(a,p+o+2*j));
     const stage=row(0x5c,h.u16(a,p+228),20);
     let channels=idx(12,4).map(k=>{let v=row(0x28,k,8);return{lighting:v[0],material_source:v[1],light_mask:v[2],diffuse:v[3],attenuation:v[4],ambient_source:v[5]};});
     let v={
       index:i,remap:map,name,mode:a[p],channels,
       material_rgba:idx(8,2).map(k=>row(0x20,k,4)),ambient_rgba:idx(20,2).map(k=>row(0x2c,k,4)),
       color_channel_count:row(0x24,a[p+2],1)[0],texgen_count:row(0x34,a[p+3],1)[0],
       stage_count:row(0x58,a[p+4],1)[0],texture_index:h.u16(a,table(0x48)+2*h.u16(a,p+132)),
       tev_order:row(0x4c,h.u16(a,p+188),4).slice(0,3),
       tev:{color_inputs:stage.slice(1,5),color_op:stage.slice(5,10),alpha_inputs:stage.slice(10,14),alpha_op:stage.slice(14,19)},
       konst_color_sel:a[p+156],konst_alpha_sel:a[p+172],
       konst_rgba:idx(148,4).map(k=>row(0x54,k,4)),
       tev_registers:idx(220,4).map(k=>[0,1,2,3].map(j=>{let n=h.u16(a,table(0x50)+k*8+j*2);return n>=32768?n-65536:n;})),
       swap_mode:row(0x60,h.u16(a,p+260),4).slice(0,2),
       swap_tables:idx(292,4).map(k=>row(0x64,k,4)),
       alpha_compare:row(0x6c,h.u16(a,p+326),8).slice(0,5),
       blend:row(0x70,h.u16(a,p+328),4),zmode:row(0x74,a[p+6],4).slice(0,3),
       zcomp:row(0x78,a[p+5],1)[0],dither:row(0x7c,a[p+7],1)[0],cull:h.u32(a,table(0x1c)+4*a[p+1])
     };mats.push(v);
   }
   let v=blocks.VTX1, q=h.u32(v,8),formats=[];
   while(h.u32(v,q)!==255){formats.push({attr:h.u32(v,q),count:h.u32(v,q+4),type:h.u32(v,q+8),frac:v[q+12]});q+=16;if(formats.length>30)throw Error("VTX list");}
   let inf=blocks.INF1,p=h.u32(inf,20),stack=[],active=null,mapping=[];
   for(let k=0;k<1000;k++){let kind=h.u16(inf,p),index=h.u16(inf,p+2);p+=4;if(kind===0)break;if(kind===1)stack.push(active);else if(kind===2){if(!stack.length)throw Error("INF nesting");active=stack.pop();}else if(kind===0x11)active=index;else if(kind===0x12)mapping.push({shape:index,material:active});else if(kind!==0x10)throw Error("INF node");}
   let shp=blocks.SHP1,shapes=[];
   for(let j=0;j<h.u16(shp,8);j++){const ii=h.u32(shp,12)+h.u16(shp,h.u32(shp,16)+j*2)*40;let vo=h.u32(shp,24)+h.u16(shp,ii+4),attrs=[];while(h.u32(shp,vo)!==255){attrs.push({attr:h.u32(shp,vo),type:h.u32(shp,vo+4)});vo+=8;if(attrs.length>32)throw Error("VCD list");}shapes.push({shape:j,attributes:attrs});}
   let tex=blocks.TEX1, textures=[];
   for(let j=0;j<h.u16(tex,8);j++){let o=h.u32(tex,12)+32*j, nt=h.u32(tex,16);textures.push({index:j,name:h.cstr(tex,nt+h.u16(tex,nt+6+j*4)),format:tex[o],width:h.u16(tex,o+2),height:h.u16(tex,o+4),image_offset:o+h.u32(tex,o+28),mip_count:tex[o+24]});}
   const no_color_attrs=formats.every(v=>v.attr!==11&&v.attr!==12)&&shapes.every(v=>v.attributes.every(x=>x.attr!==11&&x.attr!==12))&&h.u32(v,24)===0&&h.u32(v,28)===0;
   if(!no_color_attrs)throw Error("Expected absence of vertex colors changed");
   if(f.id===3&&(JSON.stringify(mapping)!==JSON.stringify([{shape:0,material:0},{shape:2,material:1},{shape:1,material:2}])||mats.map(x=>x.name).join(",")!=="oc_body_mat01,oc_body_mat02,oc_eye_mat"))throw Error("Body/eyes mapping mismatch");
   models.push({id:f.id,name:f.name,bytes:f.bytes,sha256:h.sha256(b),crc32:h.crc32(b),block_names:Object.keys(blocks),no_color_attrs,formats,mapping,shapes,textures,materials:mats});
 }
 return{status:"PASS_RESOURCE_STATIC",input_commit:"6b32ef995814e6a20924d6b6dd3fe5886a5ef3e0",arc_bytes:raw.length,arc_sha256:h.sha256(raw),arc_git_blob:blob,arc_crc32:h.crc32(raw),rarc_bytes:y.data.length,rarc_sha256:h.sha256(y.data),yaz0_consumed:y.consumed,yaz0_trailing_bytes:y.trailing,models,game_executed:false,visual:"HUMAN_RUNTIME_NOT_TESTED",original_texels_or_asset_bytes_published:false};
}
const result=parseAll(fs.readFileSync(process.argv[2])); console.log("EOC2_RESOURCE_RESULT="+JSON.stringify(result));
