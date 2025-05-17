struct t_cam_move_buf{
  int n=0;
  vector<vec2d> arr;
};
void cam_kb_commit(t_cam&cam,t_cam_move_buf&buf){
  if(buf.arr.empty())return;
  struct{vec2d v;vec2d pos;} obj={vec2d(1,0)};
  cam.pos+=buf.arr.back().Rot(cam.rot?cam.dir.Rot(-obj.v.Ort()).Norm():cam.dir)*(5.0/cam.scale);
  buf.arr.clear();
}
static void cam_kb_move(t_cam&cam,const QapKeyboard&kb,t_cam_move_buf&buf)
{
  if(cam.rot)
  {
    auto k=(1.0+(45-10)/75.0);
    real da=0;
    if(kb.Down('Q'))da=-1;
    if(kb.Down('E'))da=+1;
    cam.dir=Vec2dEx(cam.dir.GetAng()+k*(da*Pi/75.0),1);
  }
  if(bool end=true){
    auto tmp=kb.get_dir_from_wasd_and_arrows();
    if(qap_find_val(buf.arr,tmp).empty()){
      buf.arr.push_back(tmp);
    }
  }else{
    cam.pos=vec2d_zero;
  }
  if(kb.Down(VK_ADD))cam.scale*=1.01;
  if(kb.Down(VK_SUBTRACT))cam.scale/=1.01;
  if(kb.Down(VK_DIVIDE))cam.scale=1;
  if(kb.OnDown(VK_MULTIPLY))cam.scale*=0.5;
  if(kb.Down(VK_SHIFT)&&kb.OnDown(VK_DIVIDE))cam.pos=vec2d_zero;
  if(kb.Down(VK_SHIFT)&&kb.OnDown(VK_MULTIPLY))cam.pos=vec2d_zero;
}
#define OFFCENTRIC_SCOPE_IMPL(cam)struct{vec2d v;vec2d pos;} camobj={vec2d(1,0)};t_offcentric_scope scope(qDev,camobj.pos+cam.pos,cam.rot?cam.dir.Rot(-camobj.v.Ort()).Norm():cam.dir,cam.scale,cam.offcentric);