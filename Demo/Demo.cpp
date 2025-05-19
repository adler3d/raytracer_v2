#define DEF_PRO_TEMPLATE_DATAIO
#include "qaplite_without_windows.h"
#include <iostream>
#include <string>
#include <vector>
#include <string>
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
void load_mesh_obj(t_mesh&out,const string&fn){
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
    
  vec3f avg_pos;
  QAP_FOREACH(out.VA,avg_pos+=ex);
  if(!out.VA.empty())avg_pos*=1.0/out.VA.size();
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
  void add(const t_obj&obj,int model_id){
    objs.push_back({obj,model_id});
    objs.back().update_trigons();
  }
  //...
  struct t_raycast_result{
    vec3f pos,n;
    QapColor color;
    int model_id=0;
    float t=-1;
  };
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
void render(const t_obj&ground,const t_obj&model,const t_obj&sky,const vector<vec3f>&dirs,t_proj&proj,const string&fn){
  QapClock clock;
  enum{ground_id=0,model_id=1,sky_id=2};
  t_scene scene;
  scene.add(ground,0);
  scene.add(model,1);
  scene.add(sky,2);
  vector<t_frag> frags,frags_ground;
  frags.resize(proj.count_pixels());
  frags_ground.resize(proj.count_pixels());
  int i=0;atomic_int g_hits2=0;
  int n=proj.cy*proj.cx;
  atomic_int di=0;
  int prev_ticks=clock.MS();
  auto func=[&](int i)
  {
    int y=i/proj.cx;int x=i%proj.cx;
    auto photon=proj.get_photon(x,y);
    auto&frag=frags[i];auto&frag_ground=frags_ground[i];
    auto out=scene.do_raycast(photon);
    if(out.t<=0)return;
    int wins=0;int hit2=0;
    for(auto&dir:dirs){
      if(dot(out.n,dir)>0)continue;
      auto rc=scene.do_raycast(out.pos,dir);
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
  hdr.save_to_png(fn);
}
t_obj load_obj(const string&fn){
  t_mesh m;
  load_mesh_obj(m,fn);
  return {m};
}
t_obj generate_sky(){
  t_obj obj;
  obj.make_cube_fast(0xfffefefe,vec3f(0,1.7,00),vec3f(5,0.1,5));
  return obj;
}
vector<vec3f> load_dirs(const string&fn){
  //if(_access(fn.c_str(),0)!=0){QapDebugMsg(fn+" - not found");QapNoWay();}
  CrutchIO IO;
  IO.LoadFile(fn);
  if(IO.mem.empty()){QapDebugMsg(fn+" - not found or empty");QapNoWay();}
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
t_proj make_proj(int w,int h){
  t_proj proj;
  proj.cx=w;
  proj.cy=h;
  proj.wh=get_our_proj(w,h).get_wh();
  //obj.make_cube_fast(0xfffefefe,vec3f(0,1.7,00),vec3f(5,0.1,5));
  proj.pos=vec3f(0,1.51140523,0);
  //proj.dir=vec3f(-0.018422*0.25,-0.9969,-0.0162*0.5).Norm();
  proj.dir=vec3f(1e-6,-1,1e-9).Norm();
  proj.zn=0.015625;
  proj.pwh=proj.get_pwh();
  proj.up=vec3f(0,1,0);
  proj.ox=cross(proj.up,proj.dir).Norm();
  proj.oy=cross(proj.ox,proj.dir).Norm();
  proj.center=proj.pos+proj.dir*proj.zn;
  return proj;
}
void render_model_from_file(const string&name,int d=128){
  auto ground=load_obj("empty_ground.obj");
  for(auto&ex:ground.m.CA){ex=0xFFffffff;}
  auto model=load_obj(name+".obj");
  //for(auto&ex:model.m.VA){ex*=4*2*2;}
  auto sky=generate_sky();
  auto dirs=load_dirs("dirs.bin");
  auto proj=make_proj(d,d);
  render(ground,model,sky,dirs,proj,name+".png");
}
int get_num_threads(){
  atomic_int num_threads=0;
  #pragma omp parallel for schedule(dynamic,64)
  for(int i=0;i<64*1000;i++){
    if(num_threads==0)num_threads=omp_get_num_threads();
  }
  return num_threads;
}
int main(int argc,char *argv[]){
  cout<<"v5\n omp_get_num_threads()="<<get_num_threads()<<endl;
  if(bool hard_coded_fn=argc==1){
    render_model_from_file("MilitaryTruck",16);
  }else{
    auto arr=split(string(argv[1]),".");
    arr.pop_back();
    render_model_from_file(join(arr,"."),argc==3?stoi(argv[2]):128);
  }
  return 0;
}
