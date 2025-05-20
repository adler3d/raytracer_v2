Основная идея:
```cpp
---
2025.05.16 13:13:51.609
struct t_scene{
  struct t_mesh{
    t_obj obj;
    int model_id=0;
  };
  vector<t_mesh> objs;
  void add(const t_obj&obj,int model_id){
    objs.push_back({obj,model_id});
  }
  ...
};
void render(...){
  enum{ground_id=0,model_id=1,sky_id=2};
  t_scene scene;
  scene.add(ground,0);
  scene.add(model,1);
  scene.add(sky,2);
  vector<t_frag> frags,frags_ground;
  frags.resize(proj.count_pixels());
  frags_ground.resize(proj.count_pixels());
  foreach_pixel(proj){
    auto photon=to_photon(pixel);
    auto&frag=frags[i];auto&frag_ground=frags_ground[i];
    auto out=scene.do_raycast(photon);
    int wins=0;
    for(auto&dir:dirs){
      auto rc=scene.do_raycast(out.pos,dir);
      if(rc.model_id==sky_id)wins++;
    }
    if(out.model_id==ground_id){
      frag_ground.add(wins);
    }
    frag.add(wins,out.color);
  }
  auto hdr=to_hdr(frags);
  foreach_alpha(hdr.arr){
    alpha=hdr.wins_to_alpha(frags_ground[i].wins);
  }
  hdr.save_to_png("out.png");
}
---
2025.05.16 13:41:58.537
```
