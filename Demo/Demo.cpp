#define DEF_PRO_TEMPLATE_DATAIO
#include "qaplite_without_windows.h"
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <filesystem>
//#include <io.h>
using namespace std;

struct t_obj_face_item{int id;int tid;};
static t_obj_face_item get_obj_face_item(const string&s){
  if(string::npos==s.rfind('/')){
    t_obj_face_item out={stoi(s),-1};
    return out;
  }
  auto t=split(s,"/");
  //QapAssert(t.size()==2);
  t_obj_face_item out={stoi(t[0]),stoi(t[1])};
  return out;
}

struct t_obj_mtl_item{
  string name;
  vec3d Ka={1,1,1};
  vec3d Kd={1,1,1};
  QapColor get_diffuse()const{
    auto c=Kd*255;
    return QapColor(c.x,c.y,c.z);
  }
};
static string get_without_first_part(const vector<string>&arr){auto tmp=arr;QapPopFront(tmp);return join(tmp," ");}
static string get_fullname_of_near_file(const string&obj_fn,const string&mtl_fn)
{
  //if(split(obj_fn,"/").size()!=1)QapNoWay();
  {auto delim="/";auto a=split(obj_fn,delim);if(a.size()!=1){a.back()=mtl_fn;return join(a,delim);}}
  {auto delim="\\";auto a=split(obj_fn,delim);if(a.size()!=1){a.back()=mtl_fn;return join(a,delim);}}
  return mtl_fn;
}
struct t_obj_mtllib{
  vector<t_obj_mtl_item> arr;
  PRO_FUNCGEN_ADD_UNIQUE_OBJ_BY_FIELD_V2(t_obj_mtl_item,get,arr,string,name);
  void load(const string&fn){
    auto s=file_get_contents(fn);
    s=join(split(s,"\t"),"");
    s=join(split(s,"\r"),"");
    s=join(split(s," \n"),"\n");
    auto lines=split(s,"\n");
    t_obj_mtl_item*p=nullptr;
    for(int i=0;i<lines.size();i++){
      auto&ex=lines[i];
      auto t=split(ex," ");
      if(t.empty())continue;
      if(t[0]=="newmtl"){
        if(t.size()!=2){
          p=get(get_without_first_part(t));
        }else p=get(t[1]);
      }
      if(!p)continue;
      if(t[0]=="Ka"){
        QapAssert(t.size()==4);
        auto&k=p->Ka;
        for(int i=0;i<3;i++)(&k.x)[i]=stof(t[1+i]);
      }
      if(t[0]=="Kd"){
        QapAssert(t.size()==4);
        auto&k=p->Kd;
        for(int i=0;i<3;i++)(&k.x)[i]=stof(t[1+i]);
      }
    }
  }
};
struct t_mesh{
  vector<vec3f> VA;
  vector<int> IA;
  vector<QapColor> CA;
  vector<QapColor> FCA;
  void make_vertex_unique()
  {
    vector<int> U;U.resize(VA.size());auto out=VA;QapAssert(FCA.size());CA.resize(VA.size());
    for(int i=0;i+2<IA.size();i+=3)
    {
      auto color=FCA[i/3];
      for(int id=0;id<3;id++)
      {
        auto&A=IA[i+id];
        QapAssert(qap_check_id(VA,A));
        auto&a=VA[A];
        if(U[A]++)
        {
          A=out.size();
          qap_add_back(out)=a;
          QapAssert(CA.size()==A);
          CA.push_back(color);
          continue;
        }
        if(!qap_check_id(CA,A))QapDebugMsg("if(!qap_check_id(CA,A))\nA = "+IToS(A));
        CA[A]=color;
      }
    }
    VA=out;
  }
  /*
  void draw_without_ca(QapDev&qDev)
  {
    auto base=qDev.VPos;
    for(int i=0;i<VA.size();i++)qDev.AddVertex(VA[i]);
    for(int i=0;i<IA.size();i++)qDev.AddIndex(base+IA[i]);
  }
  void draw(QapDev&qDev)
  {
    auto base=qDev.VPos;
    if(CA.size()!=VA.size()){
      QapDebugMsg(IToS(CA.size())+"\n"+IToS(VA.size()));
      draw_without_ca(qDev);
      return;
    }
    for(int i=0;i<VA.size();i++){
      qDev.color=CA[i];
      qDev.AddVertex(VA[i]);
    }
    for(int i=0;i<IA.size();i++)qDev.AddIndex(base+IA[i]);
  }*/
  bool empty()const{return IA.empty();}
};
void load_mesh_obj(t_mesh&out,const string&fn,bool aabb){
  if(!out.IA.empty())return;
  QapColor diffuse=0xFFffffff;
  t_obj_mtllib lib;
  string mtllib;
  real scale=10.0;string shift_algo="upper_ground";bool use_per_vertex_color=false;vector<std::pair<bool,QapColor>> pvc;
  auto s=file_get_contents(fn);
  s=join(split(s,"\t"),"");
  s=join(split(s,"\r"),"");
  s=join(split(s,"v  "),"v ");
  s=join(split(s," \n"),"\n");
  auto arr=split(s,"\n");
  for(int i=0;i<arr.size();i++){
    auto&ex=arr[i];
    auto t=split(ex," ");
    if(t.empty())continue;
    if(t.size()==2)if(t[0]=="#scale"){scale=stof(t[1]);}
    if(t.size()==2)if(t[0]=="#shift_algo"){shift_algo=t[1];}
    if(t[0]=="mtllib"){
      mtllib=t.size()!=2?get_without_first_part(t):t[1];
      lib.load(get_fullname_of_near_file(fn,mtllib));
    }
    if(t[0]=="usemtl"){
      auto*p=lib.get(get_without_first_part(t));
      diffuse=p->get_diffuse();
    }
    if(t.size()==4||t.size()==4+3){
      if(t[0]=="v"){
        auto*f=&qap_add_back(out.VA).x;for(int i=0;i<3;i++)f[i]=stof(t[1+i]);
        if(t.size()==4+3){
          if(!use_per_vertex_color)pvc.resize(out.VA.size()-1,{false,0});
          use_per_vertex_color=true;
          auto&[ok,vc]=qap_add_back(pvc);ok=true;
          auto*p=&vc.b;
          for(int i=0;i<3;i++)p[i]=SToI(t[4+i]);
          /*hm... CA[vertex_id]=read_color(); //??? */
        }else if(use_per_vertex_color){auto&[ok,vc]=qap_add_back(pvc);ok=false;}
        continue;
      }
      //if(t[0]=="f"){for(int i=0;i<3;i++)qap_add_back(out.IA)=get_obj_face_item(t[1+i]).id-1;continue;}
    }
    if(t[0]=="f"&&t.size()>3)
    {
      vector<int> arr;
      struct{
        vec4i c;
        int n=0;
        QapColor get()const{
          return QapColor(c.a/n,c.r/n,c.g/n,c.b/n);}
      } tris;
      qap_add_back(out.FCA)=diffuse;
      for(int i=1;i<t.size();i++)qap_add_back(arr)=get_obj_face_item(t[i]).id-1;
      if(arr.size()!=3)QapDebugMsg("no way. // need bug with out.FCA.size();");
      for(int i=2;i<arr.size();i++){
        auto add=[&](int id){qap_add_back(out.IA)=id;if(use_per_vertex_color)if(auto&[ok,vc]=pvc[id];ok){tris.c+=vc;tris.n++;}};
        add(arr[0]);
        add(arr[i-1]);
        add(arr[i-0]);
      }
      if(use_per_vertex_color&&tris.n){
        out.FCA.back()=tris.get().swap_rg();
      }
      continue;
    }
    if(t[0]=="v"||t[0]=="f")QapDebugMsg("hm...\n"+ex);
  }
  if(out.VA.empty())return;
  auto find_center_by_sphere=[&](){  
    vec3f avg_pos;
    QAP_FOREACH(out.VA,avg_pos+=ex);
    if(!out.VA.empty())avg_pos*=1.0/out.VA.size();
    return avg_pos;
  };
  auto find_center_by_aabb=[&](){  
    if(out.VA.empty())return vec3f{};
    auto&A=out.VA;
    auto dno_xid=QAP_MINVAL_ID_OF_VEC(out.VA,+ex.x);
    auto dno_yid=QAP_MINVAL_ID_OF_VEC(out.VA,+ex.y);
    auto dno_zid=QAP_MINVAL_ID_OF_VEC(out.VA,+ex.z);
    auto top_xid=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.x);
    auto top_yid=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.y);
    auto top_zid=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.z);
    auto dno=vec3f(A[dno_xid].x,A[dno_yid].y,A[dno_zid].z);
    auto top=vec3f(A[top_xid].x,A[top_yid].y,A[top_zid].z);
    return dno+(top-dno)*0.5;
  };
  auto avg_pos=aabb?find_center_by_aabb():find_center_by_sphere();
  QAP_FOREACH(out.VA,ex-=avg_pos);
  
  auto id=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.SqrMag());
  auto m=out.VA[id].Mag();
  auto k3=0.000846*12;//k3=0.0146484375*2/m;QapDebugMsg(to_string(k3));
  if(fn=="empty_ground.obj"){
    k3=1;
  }
  auto k=5.0*k3/(m*scale);
  //if(fn=="cube.obj"||fn=="cube3.obj"||fn=="cube4.obj"||fn=="cube2.obj"){
  //  auto k2=k*0.125*0.5*0.125*0.75*0.5;
  //  //auto k3=0.0146484375*4*2*2*(fn=="cube2.obj"?4:1)/m;
  //  auto k3=0.0146484375*2/m;
  //  QAP_FOREACH(out.VA,ex*=k3);
  //  //QAP_FOREACH(out.VA,ex*=k2);
  //  //QapDebugMsg(to_string(k2));//0.00042287805518382421
  //  //QapDebugMsg(to_string(k3));
  //  auto id2=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.SqrMag());
  //  auto m2=out.VA[id2].Mag();
  //  QapDebugMsg("m2="+to_string(m2));
  //  /*k=0.0146484375/m;
  //  QAP_FOREACH(out.VA,ex*=k);
  //  auto id2=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.SqrMag());
  //  auto m2=out.VA[id2].Mag();
  //  QapDebugMsg(to_string(m2));*/
  //  /*auto id=QAP_MINVAL_ID_OF_VEC(out.VA,+ex.y);
  //  auto offset=vec3f(0,-1,0)*out.VA[id].y;
  //  QAP_FOREACH(out.VA,ex+=offset);*/
  //}
  /*if(fn=="cube2.obj"){
    QAP_FOREACH(out.VA,ex*=k*0.125*0.5);
    auto id=QAP_MINVAL_ID_OF_VEC(out.VA,+ex.y);
    auto offset=vec3f(0,-1,0)*out.VA[id].y;
    QAP_FOREACH(out.VA,ex+=offset);
  }*/
  if(fn=="tb6_Moskvitch.obj"){
    QAP_FOREACH(out.VA,ex*=k;std::swap(ex.z,ex.y));k=1;
  }
  if(fn=="sponza.obj"){
    QAP_FOREACH(out.VA,ex*=k;/*std::swap(ex.z,ex.y)*/ex.z*=-1;);
  }else{
    QAP_FOREACH(out.VA,ex*=k;std::swap(ex.z,ex.y));
  }
  
  //auto id2=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.SqrMag());
  //auto m2=out.VA[id2].Mag();
  //QapDebugMsg("m2="+to_string(m2));

  if(shift_algo=="under_ground"){
    auto id=QAP_MINVAL_ID_OF_VEC(out.VA,-ex.y);
    auto offset=vec3f(0,-1,0)*out.VA[id].y;
    QAP_FOREACH(out.VA,ex+=offset);
  }
  if(shift_algo=="upper_ground"){
    auto id=QAP_MINVAL_ID_OF_VEC(out.VA,+ex.y);
    auto offset=vec3f(0,-1,0)*out.VA[id].y;
    QAP_FOREACH(out.VA,ex+=offset);
  }
  out.make_vertex_unique();
}

struct t_obj{
  t_mesh m;
  static inline vec3f toVec3f(const vec2d&pos,float z=0){return vec3f(pos.x,pos.y,z);}
  void make_cube_fast(const QapColor&color,const vec3f&offset,const vec3f&size=vec3f_one)
  {
    vec2i arr[]={vec2i(-1,-1),vec2i(+1,-1),vec2i(+1,+1),vec2i(-1,+1)};
    auto vp=0;
    for(int i=0;i<4;i++)
    {
      auto a=toVec3f(arr[i],-1).RawMul(size*0.5)+offset;
      auto b=toVec3f(arr[i],+1).RawMul(size*0.5)+offset;
      m.VA.push_back(a);
      m.CA.push_back(color);
      m.VA.push_back(b);
      m.CA.push_back(color);
      add_quad_ex(8,i*2,vp,0,1,3,2);
    }
    add_quad(vp+0,0,2,4,6);
    add_quad(vp+1,6,4,2,0);
  }
  void add_quad(int base,int a,int b,int c,int d)
  {
    auto vp=base;
    add_tris(vp+a,vp+b,vp+c);
    add_tris(vp+c,vp+d,vp+a);
  }
  inline void add_tris(int A,int B,int C)
  {
    m.IA.push_back(A);
    m.IA.push_back(B);
    m.IA.push_back(C);
  };
  void add_quad_ex(int mod,int offset,int base,int a,int b,int c,int d)
  {
    auto vp=base;
    add_tris(vp+(a+offset)%mod,vp+(b+offset)%mod,vp+(c+offset)%mod);
    add_tris(vp+(c+offset)%mod,vp+(d+offset)%mod,vp+(a+offset)%mod);
  }
};
struct t_photon{vec3f pos,dir;};
struct t_proj{
  int cx,cy;int count_pixels(){return cx*cy;}
  vec3f pos,dir;
  vec2d wh;
  real zn;
  vec2d get_pwh(){return vec2d(1.0/cx,1.0/cy).Mul(wh);}
  vec2d pwh;
  vec3f up=vec3f(0,1,0);
  vec3f ox=vec3f(1,0,0);
  vec3f oy=vec3f(0,0,1);
  vec3f center;
  t_photon get_photon(int x,int y){
    auto offset=vec2d(x-cx/2,y-cy/2).Mul(pwh)*2+pwh;
    auto quad_center=center+ox*offset.x+oy*offset.y;
    return {quad_center,dir};
  }
};
struct vec4d{};
struct t_frag{
  int wins=0;
  vec3f color;
  void add(int wins,const QapColor&color){this->wins+=wins;this->color+=vec3f(color.r,color.g,color.b)*(wins*1.0/255);}
};
 struct t_scene{
  struct t_trigon_vec3f{
    vec3f a,b,c;
  };
  struct t_mesh{
    t_obj obj;
    int model_id=0;
    vector<t_trigon_vec3f> trigons;
    vector<QapColor> colors;
    void update_trigons(){
      trigons.resize(obj.m.IA.size()/3);
      colors.resize(obj.m.IA.size()/3);
      for(int i=2;i<obj.m.IA.size();i++){
        auto&a=obj.m.VA[obj.m.IA[i-2]];
        auto&b=obj.m.VA[obj.m.IA[i-1]];
        auto&c=obj.m.VA[obj.m.IA[i-0]];
        trigons[i/3]={a,b,c};
        colors[i/3]=obj.m.CA[obj.m.IA[i-0]];
      }
    }
  };
  vector<t_mesh> objs;
  t_mesh last;float sky_miny=0;
  
  static float miny(const t_obj&obj){
    int id=QAP_MINVAL_ID_OF_VEC(obj.m.VA,ex.y);
    return id<0?0:obj.m.VA[id].y;
  }
  void add(const t_obj&obj,int model_id){
    objs.push_back({obj,model_id});
    objs.back().update_trigons();
    last=objs.back();
  }
  //...
  struct t_raycast_result_v2{
    int model_id=0;
    float t=-1;
  };
  float ray_vs_plane(const vec3f&normal,const vec3f&center,const vec3f&ray_dir,const vec3f&ray_origin){
    float denom=normal.dot(ray_dir);
    if(abs(denom)>0.0001f){
      float t=(center-ray_origin).dot(normal)/denom;
      return t;
    }
    return -1;
  }
  t_raycast_result_v2 do_raycast_v3(const t_photon&photon){return do_raycast_v3(photon.pos,photon.dir);}
  t_raycast_result_v2 do_raycast_v3(const vec3f&pos,const vec3f&dir){
    t_raycast_result_v2 out;
    auto tg=ray_vs_plane({0,1,0},{0,0,0},dir,pos);
    out={0,tg};
    for(int i=0;i<last.trigons.size();i++){
      auto&it=last.trigons[i];
      auto t=raycast_to_trigon_v2(pos,dir,it.a,it.b,it.c);
      if(t<0)continue;
      if(out.t>0&&out.t<t)continue;
      out.t=t;
      out.model_id=last.model_id;
    }
    auto ts=ray_vs_plane({0,-1,0},{0,sky_miny,0},dir,pos);
    if(ts<0)return out;
    return out.t>0&&out.t<ts?out:t_raycast_result_v2{2,ts};
  }
  t_raycast_result_v2 do_raycast_v2(const t_photon&photon){return do_raycast_v2(photon.pos,photon.dir);}
  t_raycast_result_v2 do_raycast_v2(const vec3f&pos,const vec3f&dir){
    t_raycast_result_v2 out;
    for(auto&ex:objs){
      for(int i=0;i<ex.trigons.size();i++){
        auto&it=ex.trigons[i];
        auto t=raycast_to_trigon_v2(pos,dir,it.a,it.b,it.c);
        if(t<0)continue;
        if(out.t>0&&out.t<t)continue;
        out.t=t;
        out.model_id=ex.model_id;
      }
    }
    return out;
  }
  struct t_raycast_result{
    vec3f pos,n;
    QapColor color;
    int model_id=0;
    float t=-1;
  };
  t_raycast_result do_raycast_full(const t_photon&photon){return do_raycast_full(photon.pos,photon.dir);}
  t_raycast_result do_raycast_full(const vec3f&pos,const vec3f&dir){
    t_raycast_result out;
    auto tg=ray_vs_plane({0,-1,0},{0,0,0},dir,pos);
    out={pos+dir*tg,{0,-1,0},0xffffffff,0,tg};
    for(int i=0;i<last.trigons.size();i++){
      auto&it=last.trigons[i];
      auto rr=raycast_to_trigon(pos,dir,it.a,it.b,it.c);
      if(rr.t<0)continue;
      if(out.t>0&&out.t<rr.t)continue;
      out=rr;
      out.color=last.colors[i];
      out.model_id=last.model_id;
    }
    auto ts=ray_vs_plane({0,+1,0},{0,sky_miny,0},dir,pos);
    if(ts<0)return out;
    return out.t>0&&out.t<ts?out:t_raycast_result{pos+dir*ts,{0,+1,0},0xffffffff,2,ts};
  }
  t_raycast_result do_raycast(const t_photon&photon){return do_raycast(photon.pos,photon.dir);}
  t_raycast_result do_raycast(const vec3f&pos,const vec3f&dir){
    t_raycast_result out;
    for(auto&ex:objs){
      for(int i=0;i<ex.trigons.size();i++){
        auto&t=ex.trigons[i];
        auto rr=raycast_to_trigon(pos,dir,t.a,t.b,t.c);
        if(rr.t<0)continue;
        if(out.t>0&&out.t<rr.t)continue;
        out=rr;
        out.color=ex.colors[i];
        out.model_id=ex.model_id;
      }
    }
    return out;
  }
  static inline t_raycast_result raycast_to_trigon(const vec3f&pos,const vec3f&dir,const vec3f&a,const vec3f&b,const vec3f&c){
    //auto&cross=cross;auto&dot=dot;
    t_raycast_result nope;
    auto center=a;//(a+b+c)*(1.0/3.0);
    auto n=cross(b-a,c-a).Norm();
    if(dot(n,dir)<0)return nope;
    auto oy=-n;
    auto oz=cross(dir,n).Norm();
    auto ox=cross(oz,n).Norm();
    auto py=dot(oy,pos-center);
    auto dy=dot(oy,dir);
    if(!dy)return nope;
    if(py/dy>0)return nope;
    auto point=pos-(dir*(py/dy));
    #define F(A,a)auto A=vec2d(dot(ox,a),dot(oz,a));
    F(A,a);F(B,b);F(C,c);F(P,point);
    #undef F
    int t[3]={0,0,0};
    #define F(A,B)t[int(Sign(cross(B-A,P-A))+1)]++;
    F(A,B);F(B,C);F(C,A);
    #undef F
    if(t[0]&&t[2])return nope;
    t_raycast_result out;out.pos=point;out.n=n;out.t=abs(py/dy);
    return out;
  }
  float raycast_to_trigon_v2(const vec3f&pos,const vec3f&dir,const vec3f&A,const vec3f&B,const vec3f&C){
    const float EPSILON=0.000001f;
    vec3f edge1,edge2,h,s,q;
    float a,f,u,v;
    edge1=B-A;
    edge2=C-A;
    h=cross(dir,edge2);
    a=dot(edge1,h);
    if(a>-EPSILON&&a<EPSILON)return -1;
    f=1.0f/a;
    s=pos-A;
    u=f*dot(s,h);
    if(u<0.0f||u>1.0f)return -1;
    q=cross(s,edge1);
    v=f*dot(dir,q);
    if(v<0.0f||u+v>1.0f)return -1;
    auto t=f*dot(edge2,q);
    return t>EPSILON?t:-1;
  }
};
static vec3d to_vec3d(const QapColor&c){return vec3d(c.r,c.g,c.b)*(1.0/255.0);}
static vec3d vec3d_pow(const vec3d&c,real v){return vec3d(pow(c.x,v),pow(c.y,v),pow(c.z,v));}
static vec3f vec3f_pow(const vec3f&c,float v){return vec3f(pow(c.x,v),pow(c.y,v),pow(c.z,v));}
#pragma region do_clamp
static vector<real> do_clamp_bottom_bound(const vector<real>&arr,real portion=0.1)
{
  real low=1e20;real top=-low;
  int id=QAP_MINVAL_ID_OF_VEC(arr,+ex);auto v=id<0?top:arr[id];
  QAP_FOREACH(arr,if(ex>v)low=std::min<real>(ex,low));
  auto out=arr;
  QAP_FOREACH(out,if(ex<=v)ex=low);
  return out;
}
static vector<real> do_clamp_upper_bound(const vector<real>&arr,real portion=0.1)
{
  real low=1e20;real top=-low;
  int id=QAP_MINVAL_ID_OF_VEC(arr,-ex);auto v=id<0?low:arr[id];
  QAP_FOREACH(arr,if(ex<v)top=std::max<real>(ex,top));
  auto out=arr;
  QAP_FOREACH(out,if(ex>=v)ex=top);
  return out;
}
static real vec3d_max(const vec3d&v){return std::max(std::max(v.x,v.y),v.z);}
static real vec3d_min(const vec3d&v){return std::min(std::min(v.x,v.y),v.z);}
static vec3d vec3d_max(const vec3d&a,const vec3d&b){return vec3d_max(a)<vec3d_max(b)?b:a;}
static vec3d vec3d_min(const vec3d&a,const vec3d&b){return vec3d_min(a)>vec3d_min(b)?b:a;}
static real vec3f_max(const vec3f&v){return std::max(std::max(v.x,v.y),v.z);}
static real vec3f_min(const vec3f&v){return std::min(std::min(v.x,v.y),v.z);}
static vec3f vec3f_max(const vec3f&a,const vec3f&b){return vec3f_max(a)<vec3f_max(b)?b:a;}
static vec3f vec3f_min(const vec3f&a,const vec3f&b){return vec3f_min(a)>vec3f_min(b)?b:a;}
static vector<vec3d> do_clamp_bottom_bound(const vector<vec3d>&arr)
{
  auto out=arr;
  real low=1e20;real top=-low;bool fail=true;
  int id=QAP_MINVAL_ID_OF_VEC(arr,+vec3d_min(ex));auto v=id<0?top:vec3d_min(arr[id]);
  QAP_FOREACH(arr,auto m=vec3d_min(ex);if(m>v){low=std::min<real>(m,low);fail=false;});
  if(fail)return out;
  #define F(x)if(ex.x<=v)ex.x=low;
  QAP_FOREACH(out,F(x)F(y)F(z));
  #undef F
  return out;
}
static vector<vec3d> do_clamp_upper_bound(const vector<vec3d>&arr)
{
  auto out=arr;
  real low=1e20;real top=-low;bool fail=true;
  int id=QAP_MINVAL_ID_OF_VEC(arr,-vec3d_max(ex));auto v=id<0?low:vec3d_max(arr[id]);
  QAP_FOREACH(arr,auto m=vec3d_max(ex);if(m<v){top=std::max<real>(m,top);fail=false;});
  if(fail)return out;
  #define F(x)if(ex.x>=v)ex.x=top;
  QAP_FOREACH(out,F(x)F(y)F(z));
  #undef F
  return out;
}
static vector<t_frag> do_clamp_bottom_bound(const vector<t_frag>&arr)
{
  auto out=arr;
  real low=1e20;real top=-low;bool fail=true;
  int id=QAP_MINVAL_ID_OF_VEC(arr,+vec3f_min(ex.color));auto v=id<0?top:vec3f_min(arr[id].color);
  QAP_FOREACH(arr,auto m=vec3f_min(ex.color);if(m>v){low=std::min<real>(m,low);fail=false;});
  if(fail)return out;
  #define F(x)if(ex.color.x<=v)ex.color.x=low;
  QAP_FOREACH(out,F(x)F(y)F(z));
  #undef F
  return out;
}
static vector<t_frag> do_clamp_upper_bound(const vector<t_frag>&arr)
{
  auto out=arr;
  real low=1e20;real top=-low;bool fail=true;
  int id=QAP_MINVAL_ID_OF_VEC(arr,-vec3f_max(ex.color));auto v=id<0?low:vec3f_max(arr[id].color);
  QAP_FOREACH(arr,auto m=vec3f_max(ex.color);if(m<v){top=std::max<real>(m,top);fail=false;});
  if(fail)return out;
  #define F(x)if(ex.color.x>=v)ex.color.x=top;
  QAP_FOREACH(out,F(x)F(y)F(z));
  #undef F
  return out;
}
/*static vector<vec3d> do_clamp_upper_bound(const vector<vec3d>&arr)
{
  QapNoWay();
  auto out=arr;
  auto low=vec3d(1,1,1)*1e20;auto top=-low;vec3d v;
  #define F(x){int id=QAP_MINVAL_ID_OF_VEC(arr,-ex.x);v.x=id<0?low.x:arr[id].x;}QAP_FOREACH(arr,if(ex.x<v.x)top.x=std::max<real>(ex.x,top.x));
  F(x)F(y)F(z)
  #undef F
  real vm=vec3d_max(v);
  #define F(x)if(v.x==vm)QAP_FOREACH(out,if(ex.x>=v.x)ex.x=top.x);
  F(x)F(y)F(z)
  #undef F
  return out;
}*/
static vector<vec3d> hdr_pixmap(const vector<vec3d>&pix,real min=0x00,real max=0xff)
{
  real low=1e20;real top=-low;
  QAP_FOREACH(pix,top=std::max<real>(vec3d_max(ex),top);low=std::min<real>(vec3d_min(ex),low));
  real dip=max-min;
  real koef=(dip/(top-low));
  auto dno=vec3d(1,1,1)*low;auto min3d=vec3d(min,min,min);
  auto out=pix;
  QAP_FOREACH(out,ex=min3d+(ex-dno)*koef);
  return out;
}
static vector<t_frag> hdr_pixmap(const vector<t_frag>&pix,float&koef,vec3f&dno,vec3f&min3f,float min=0x00,float max=0xff)
{
  float low=1e20;float top=-low;
  QAP_FOREACH(pix,top=std::max<real>(vec3f_max(ex.color),top);low=std::min<real>(vec3f_min(ex.color),low));
  float dip=max-min;
  koef=(dip/(top-low));
  dno=vec3f(1,1,1)*low;min3f=vec3f(min,min,min);
  auto out=pix;
  QAP_FOREACH(out,ex.color=min3f+(ex.color-dno)*koef);
  {
    float low=1e20;float top=-low;
    QAP_FOREACH(out,top=std::max<real>(vec3f_max(ex.color),top);low=std::min<real>(vec3f_min(ex.color),low));
    //QapDebugMsg(to_string(low)+" "+to_string(top));
  }
  return out;
}
static vector<real> hdr_pixmap(const vector<real>&pix,real min=0x00,real max=0xff/*,bool use_some_hack_with_limits=false*/)
{
  real low=1e20;real top=-low;
  //int topid=QAP_MINVAL_ID_OF_VEC(pix,-ex);auto topv=topid<0?low:pix[topid];
  //int lowid=QAP_MINVAL_ID_OF_VEC(pix,+ex);auto lowv=lowid<0?top:pix[lowid];
  //QAP_FOREACH(pix,if(ex<topv)top=std::max<real>(ex,top);if(ex>lowv)low=std::min<real>(ex,low));
  QAP_FOREACH(pix,top=std::max<real>(ex,top);low=std::min<real>(ex,low));
  real dip=max-min;
  real koef=(dip/(top-low));
  auto out=pix;
  QAP_FOREACH(out,ex=min+(ex-low)*koef);
  return out;
}
static vector<QapColor> hdr_pixmap(const vector<QapColor>&pix)
{
  int low=256;int top=-1;
  QAP_FOREACH(pix,if(ex.r!=255)top=std::max<int>(ex.r,top);if(ex.r)low=std::min<int>(ex.r,low));
  //low--;
  real koef=low>255?1:(253.0/(top-low));
  auto f=[low,top,koef](const QapColor&c){
    auto out=c;
    #define F(r)out.r=c.r==255?255:(!c.r?0:1+std::min<int>(253,(c.r-low)*koef));
    F(r)F(g)F(b)
    #undef F
    return out;
  };
  auto out=pix;
  QAP_FOREACH(out,ex=f(ex));
  return out;
}
#pragma endregion
string remove_file_ext(string fn){
  auto arr=split(fn,".");
  arr.pop_back();
  return join(arr,".");
}
string get_file_ext(string fn){
  auto arr=split(fn,".");
  return arr.back();
}
#include "lodepng.cpp"
static void lodepng_encodeOneStep(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height)
{
  unsigned error=lodepng::encode(filename, image, width, height);
  if(error)QapDebugMsg("encoder error "+to_string(error)+": "+lodepng_error_text(error));
}
void lodepng_save_to_png(int cx,int cy,const vector<QapColor>&arr,const char*fn)
{
  unsigned width = cx, height = cy;
  std::vector<unsigned char> image;
  image.resize(width * height * 4);
  for(unsigned y = 0; y < height; y++)
  for(unsigned x = 0; x < width; x++)
  {
    auto c=arr[x+cx*y];
    auto&out=*(QapColor*)(void*)&image[4 * width * y + 4 * x + 0];
    out=c.swap_rg();
  }
  lodepng_encodeOneStep(fn, image, width, height);
}
struct t_hdr{
  vector<QapColor> arr;
  float koef=0;
  vec3f dno,min3f;
  int cx,cy;
  unsigned char wins_to_alpha(int wins,int i){
    static constexpr real inv_gamma=1.0/2.2;
    auto w=pow(float(wins),inv_gamma);
    auto c=(min3f.x+(w-dno.x)*koef);
    if(c>255)QapDebugMsg("arr.size()="+to_string(arr.size())+";i="+to_string(i)+";koef="+to_string(koef)+";wins="+to_string(wins)+";c="+to_string(c));
    return 255-c;
  }
  void save_to_png(const string&fn){
    lodepng_save_to_png(cx,cy,arr,fn.c_str());
  }
};
t_hdr to_hdr(const vector<t_frag>&frags,int cx,int cy){
  t_hdr out;out.cx=cx;out.cy=cy;
  auto farr=frags;
  static const real inv_gamma=1.0/2.2;
  QAP_FOREACH(farr,ex.color=vec3f_pow(ex.color,inv_gamma));
  auto old=farr;
  //farr=do_clamp_bottom_bound(farr);
  //farr=do_clamp_upper_bound(farr);
  //QAP_FOREACH(farr,QapAssert(ex.color==old[i].color));
  farr=hdr_pixmap(farr,out.koef,out.dno,out.min3f,0.0,255.0);
  out.arr.resize(farr.size());
  QAP_FOREACH(farr,out.arr[i]=QapColor(ex.color.x,ex.color.y,ex.color.z));
  return out;
}
struct t_hdr_v2{
  vector<vector<QapColor>> arr;
  float koef=0;
  vec3f dno,min3f;
  unsigned char wins_to_alpha(int wins){
    static constexpr real inv_gamma=1.0/2.2;
    auto w=pow(float(wins),inv_gamma);
    auto c=(min3f.x+(w-dno.x)*koef);
    return 255-c;
  }
};
static void hdr_pixmap_v2(vector<vector<vec3f>>&pix,float&koef,vec3f&dno,vec3f&min3f,float min=0x00,float max=0xff)
{
  vector<vec3f> pixtmp;
  for(auto&it:pix)QAP_FOREACH(it,pixtmp.push_back(ex));
  vector<float> x,y,z;x.resize(pixtmp.size());y=x;z=x;
  QAP_FOREACH(pixtmp,x[i]=ex.x);
  QAP_FOREACH(pixtmp,y[i]=ex.y);
  QAP_FOREACH(pixtmp,z[i]=ex.z);
  std::sort(x.begin(),x.end());
  std::sort(y.begin(),y.end());
  std::sort(z.begin(),z.end());
  auto d=0.007;
  auto lowx=x[int(x.size()*d)];auto topx=x[int(x.size()*(1.0-d))];
  auto lowy=y[int(y.size()*d)];auto topy=y[int(y.size()*(1.0-d))];
  auto lowz=z[int(z.size()*d)];auto topz=z[int(z.size()*(1.0-d))];
  auto low=std::min({lowx,lowy,lowz});
  auto top=std::max({topx,topy,topz});
  float dip=max-min;
  koef=(dip/(top-low));
  dno=vec3f(1,1,1)*low;min3f=vec3f(min,min,min);
  auto f=[&](vec3f v){if(v.x<0)v.x=0;if(v.y<0)v.y=0;if(v.z<0)v.z=0;return v;};
  auto g=[&](vec3f v){if(v.x>max)v.x=max;if(v.y>max)v.y=max;if(v.z>max)v.z=max;return v;};
  for(auto&it:pix)QAP_FOREACH(it,ex=g(min3f+f(ex-dno)*koef));
  //{
  //  float low=1e20;float top=-low;
  //  for(auto&it:pix)QAP_FOREACH(it,top=std::max<real>(vec3f_max(ex),top);low=std::min<real>(vec3f_min(ex),low));
  //  QapDebugMsg(to_string(low)+" "+to_string(top));
  //}
}
t_hdr_v2 to_hdr_v2(const vector<vector<t_frag>>&arr){
  t_hdr_v2 out;
  vector<vector<vec3f>> tmp;
  out.arr.resize(arr.size());tmp.resize(arr.size());
  static const real inv_gamma=1.0/2.2;
  for(int i=0;i<arr.size();i++){
    auto&inp=arr[i];
    auto&t=tmp[i];
    t.resize(inp.size());
    QAP_FOREACH(inp,t[i]=vec3f_pow(ex.color,inv_gamma));
  }
  hdr_pixmap_v2(tmp,out.koef,out.dno,out.min3f,0.0,255.0);
  for(int i=0;i<tmp.size();i++)out.arr[i].resize(tmp[i].size());
  for(int j=0;j<tmp.size();j++){
    auto&r=out.arr[j];
    QAP_FOREACH(tmp[j],r[i]=QapColor(ex.x,ex.y,ex.z));
  }
  return out;
}
void save_frags(const string&name,const vector<t_frag>&frags,const vector<t_frag>&frags_ground,int cx,int cy){
  CrutchIO IO;
  IO.write((char*)&cx,sizeof(cx));
  IO.write((char*)&cy,sizeof(cy));
  for(auto&ex:frags){
    IO.write((char*)&ex.color,sizeof(ex.color));
  }
  for(auto&ex:frags_ground){
    IO.write((char*)&ex.wins,sizeof(ex.wins));
  }
  IO.SaveFile(name+".qaphdr");
}
struct t_frags{
  string fn;
  vector<t_frag> frags;
  vector<int> frags_ground;
  int cx{},cy{};
  void load(const string&fn){
    this->fn=fn;
    CrutchIO IO;
    IO.LoadFile(fn);
    if(IO.mem.empty())QapDebugMsg("file not found or empty- "+ fn);
    IO.read((char*)&cx,sizeof(cx));
    IO.read((char*)&cy,sizeof(cy));
    frags.resize(cx*cy);
    for(auto&ex:frags){
      IO.read((char*)&ex.color,sizeof(ex.color));
    }
    frags_ground.resize(frags.size());
    for(auto&ex:frags_ground){
      IO.read((char*)&ex,sizeof(ex));
    }
  }
};
void make_results(const vector<string>&fns,bool use_ca2=false){
  vector<t_frags> arr;
  arr.resize(fns.size());
  vector<vector<t_frag>> tmp;tmp.resize(arr.size());
  for(int i=0;i<arr.size();i++){
    auto&ex=arr[i];
    ex.load(fns[i]);
    cout<<("loading "+fns[i])<<endl;
    tmp[i]=ex.frags;
  }
  auto h2=to_hdr_v2(tmp);
  for(int i=0;i<arr.size();i++){
    auto&ex=arr[i];
    auto&ca=h2.arr[i];
    auto ca2=ca;
    auto fn=remove_file_ext(fns[i])+string(use_ca2?".front":"")+".png";
    auto fn2=remove_file_ext(fns[i])+".shadow.png";
    cout<<("saving to "+fn)<<endl;
    for(int i=0;i<ca.size();i++){
      //auto&it=ca[i];
      auto&it=use_ca2?ca2[i]:ca[i];
      auto&fg=ex.frags_ground[i];
      if(fg!=0){
        it.a=h2.wins_to_alpha(fg);
        it.r=0;it.g=0;it.b=0;
        if(use_ca2)ca[i]=0;
      }else{
        if(use_ca2)it=0;
      }
    }
    if(bool need_alpha_circle=true){
      int n=ex.cx*ex.cy;
      auto out=use_ca2?ca2:ca;
      for(int i=0;i<n;i++){
        int y=i/ex.cx;int x=i%ex.cx;
        if(vec2d(x-ex.cx*0.5,y-ex.cy*0.5).Mag()<ex.cx*0.5)continue;
        out[i].a=0;
      }
    }
    lodepng_save_to_png(ex.cx,ex.cy,ca,fn.c_str());
    if(use_ca2)lodepng_save_to_png(ex.cx,ex.cy,ca2,fn2.c_str());
  }
}
void render(const t_obj&ground,const t_obj&model,const t_obj&sky,const vector<vec3f>&dirs,t_proj&proj,const string&name){
  QapClock clock;
  enum{ground_id=0,model_id=1,sky_id=2};
  t_scene scene;
  //scene.add(ground,ground_id);
  scene.add(model,model_id);
  //scene.add(sky,sky_id);
  scene.sky_miny=t_scene::miny(sky);
  vector<t_frag> frags,frags_ground;
  frags.resize(proj.count_pixels());
  frags_ground.resize(proj.count_pixels());
  int i=0;atomic_int g_hits2=0;
  int n=proj.cy*proj.cx;
  atomic_int di=0;
  auto prev_ticks=clock.MS();
  auto func=[&](int i)
  {
    //cout<<i<<endl;
    int y=i/proj.cx;int x=i%proj.cx;
    auto photon=proj.get_photon(x,y);
    auto&frag=frags[i];auto&frag_ground=frags_ground[i];
    auto out=scene.do_raycast_full(photon);
    if(out.t<=0)return;
    int wins=0;int hit2=0;
    for(auto&dir:dirs){
      if(dot(out.n,dir)>0)continue;
      auto rc=scene.do_raycast_v3(out.pos,dir);
      if(rc.t<=0)continue;
      if(rc.model_id==sky_id)wins++;
      if(rc.model_id==model_id){
        hit2++;
        //QapDebugMsg("hit2");
      }
    }
    if(out.model_id==ground_id){
      frag_ground.wins+=wins;
    }
    if(out.model_id==model_id){
      //QapDebugMsg("hit");
    }
    if(out.model_id==sky_id){
      QapDebugMsg("hit_sky");
    }
    g_hits2+=hit2;
    frag.add(wins,out.color);
    di++;
    if(di%proj.cx==proj.cx-1){
      #pragma omp critical(nope)
      {
        auto ms=clock.MS();
        auto dt=(ms-prev_ticks)*0.001;prev_ticks=ms;
        auto t=ms*0.001;
        auto ste=t*(n-di)/(di+1);
        auto ste2=(n-di)*dt/proj.cx;
        cout<<"["<<to_string(t)<<" sec]: "<<y<<"/"<<proj.cy<<" // "<<ste<<" sec till end. // ste2="<<ste2<<endl;
      }
    }
  };
  #ifdef _DEBUG
    for(int i=0;i<n;i++)func(i);
  #else
    #pragma omp parallel for schedule(dynamic,64)
    for(int i=0;i<n;i++)func(i);
  #endif
  //QapDebugMsg(to_string(g_hits2));
  save_frags(name,frags,frags_ground,proj.cx,proj.cy);
  auto hdr=to_hdr(frags,proj.cx,proj.cy);
  for(int i=0;i<hdr.arr.size();i++){
    auto&ex=hdr.arr[i];
    auto&fg=frags_ground[i];
    if(fg.wins!=0){
      ex.a=hdr.wins_to_alpha(fg.wins,i);
      ex.r=0;ex.g=0;ex.b=0;
    }
  }
  if(bool need_alpha_circle=true){
    for(int i=0;i<n;i++){
      int y=i/proj.cx;int x=i%proj.cx;
      if(vec2d(x-proj.cx*0.5,y-proj.cy*0.5).Mag()<proj.cx*0.5)continue;
      hdr.arr[i].a=0;
    }
  }
  hdr.save_to_png(name+".png");
}
t_obj load_obj(const string&fn,bool aabb){
  t_mesh m;
  load_mesh_obj(m,fn,aabb);
  return {m};
}
t_obj generate_sky(){
  t_obj obj;
  obj.make_cube_fast(0xfffefefe,vec3f(0,1.7,00),vec3f(5,0.1,5));
  return obj;
}
vector<vec3f> load_dirs(const string&fn,bool must_be_found){
  //if(_access(fn.c_str(),0)!=0){QapDebugMsg(fn+" - not found");QapNoWay();}
  CrutchIO IO;
  IO.LoadFile(fn);
  if(IO.mem.empty()){if(!must_be_found)return {};QapDebugMsg(fn+" - not found or empty");QapNoWay();}
  int count=0;
  IO.read((char*)&count,sizeof(count));
  vector<vec3f> arr;arr.resize(count);
  IO.read((char*)arr.data(),sizeof(*arr.data())*count);
  auto O=vec3f(0,0,0);QAP_FOREACH(arr,O+=ex);O*=1.0/arr.size();
  QAP_FOREACH(arr,ex=(ex-O).Norm());
  QAP_FOREACH(arr,if(fabs(ex.Mag()-1)>0.001)QapDebugMsg("t_sphere_n::dirs[i].Mag() = "+FToS(ex.Mag())););
  return arr;
}
struct t_perspective_proj{
  vec2i s;
  real fovy;
  real aspect;
  real zn;
  real zf;
  //QapMat4 get_mat()const{return MatrixPerspectiveFovLH(fovy,aspect,zn,zf);}
  vec2d get_wh()const{
    auto h=zn*tan(fovy*0.5);
    return vec2d(aspect*h,h);
  }
  t_perspective_proj zn_mul(real k)const{auto out=*this;out.zn*=k;return out;}
};
t_perspective_proj get_our_proj(int w,int h)
{
  t_perspective_proj out;
  auto s=vec2i(w,h);
  out.s=s;
  out.fovy=Pi*70/180;
  out.aspect=s.x/float(s.y);
  out.zn=1.0/64;
  out.zf=64;
  return out;
}
t_proj make_proj(int w,int h,real ang){
  t_proj proj;
  proj.cx=w;
  proj.cy=h;
  proj.wh=get_our_proj(w,h).get_wh();
  auto a=Pi/2-ang;
  proj.pos=vec3f(1.51140523*cos(a),1.51140523*sin(a),0);
  //proj.dir=vec3f(-0.018422*0.25,-0.9969,-0.0162*0.5).Norm();
  auto v=Vec2dEx(a,1);
  proj.dir=vec3f(1e-6-v.x,-v.y,1e-9).Norm();
  proj.zn=0.015625;
  proj.pwh=proj.get_pwh();
  proj.up=vec3f(0,1,0);
  proj.ox=cross(proj.up,proj.dir).Norm();
  proj.oy=cross(proj.ox,proj.dir).Norm();
  proj.center=proj.pos+proj.dir*proj.zn;
  return proj;
}
void render_model_from_file(const string&path,const string&fn,int d=128,real ang=0,bool aabb=false){
  auto ground=load_obj(path+"empty_ground.obj",false);
  if(ground.m.VA.empty())ground=load_obj("empty_ground.obj",false);
  for(auto&ex:ground.m.CA){ex=0xFFffffff;}
  auto model=load_obj(fn,aabb);
  auto sky=generate_sky();
  auto dirs=load_dirs(path+"dirs.bin",false);
  if(dirs.empty())dirs=load_dirs("dirs.bin",true);
  auto proj=make_proj(d,d,ang);
  auto name=remove_file_ext(fn);
  render(ground,model,sky,dirs,proj,name+"_"+IToS(d)+(ang?"_"+FToS(ang):""));
}
int get_num_threads(){
  atomic_int num_threads=0;
  #pragma omp parallel for schedule(dynamic,64)
  for(int i=0;i<64*1000;i++){
    if(num_threads==0)num_threads=omp_get_num_threads();
  }
  return num_threads;
}
string get_path(const string&fn){
  string s=fn;
  auto apos=s.rfind("/");
  auto bpos=s.rfind("\\");
  auto split_by=[&](const string&it){auto arr=split(s,it);arr.pop_back();s=join(arr,it)+it;};
  if(apos==string::npos&&bpos==string::npos){return "";}else if(apos!=string::npos&&bpos!=string::npos){
    if(apos>bpos)split_by("/");
    if(bpos>apos)split_by("\\");
  }else if(apos==string::npos){split_by("\\");}else split_by("/");
  return s;
}
namespace fs=std::filesystem;
bool is_dir(const string&fn){
  const fs::path path(fn);
  std::error_code ec;
  if(fs::is_directory(path,ec))return true;
  return false;
}
int main(int argc,char *argv[]){
  cout<<"v8\n omp_get_num_threads()="<<get_num_threads()<<endl;
  auto path=get_path(argv[0]);
  if(argc==1){
    cout<<"Usage: Demo model.obj 128 0\n where 128 size of image and 0 the angle at which the image will be rendered"<<endl;
    return 0;
  }else{
    cout<<"path=["<<path<<"]"<<endl;
    if(string(argv[1])=="--mr"){
      vector<string> fns;
      bool use_ca2=false;
      for(int i=2;i<argc;i++){
        string fn=argv[i];
        if(fn=="--use_ca2"){use_ca2=true;continue;}
        if(is_dir(fn)){
          std::string path=fn;
          for(const auto&entry:fs::directory_iterator(path)){
            if(fs::is_regular_file(entry)) {
              auto fn=entry.path().string();
              if(get_file_ext(fn)=="qaphdr")fns.push_back(fn);
            }
          }
        }else fns.push_back(fn);
      }
      make_results(fns,use_ca2);
    }else{
      int size=argc>=3?stoi(argv[2]):128;
      auto ang=argc>=4?stof(argv[3])*Pi/180.0:0;
      bool aabb=argc>=5?(string(argv[4])=="aabb"):false;
      render_model_from_file(path,argv[1],size,ang,aabb);
    }
  }
  return 0;
}
