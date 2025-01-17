#include "box.h"

box::box(const vec3f& p0, const vec3f& p1, std::shared_ptr<material> ptr, 
         std::shared_ptr<alpha_texture> alpha_mask, std::shared_ptr<bump_texture> bump_tex,
         std::shared_ptr<Transform> ObjectToWorld, std::shared_ptr<Transform> WorldToObject, bool reverseOrientation) :
  hitable(ObjectToWorld, WorldToObject, reverseOrientation) {
  pmin = (p0);
  pmax = (p1);
  list.add(std::make_shared<xy_rect>(p0.x(), p1.x(), p0.y(), p1.y(), p1.z(), ptr, alpha_mask, bump_tex, 
                                     ObjectToWorld, WorldToObject, false));
  list.add(std::make_shared<xy_rect>(p0.x(), p1.x(), p0.y(), p1.y(), p0.z(), ptr, alpha_mask, bump_tex, 
                                     ObjectToWorld, WorldToObject, true));
  list.add(std::make_shared<xz_rect>(p0.x(), p1.x(), p0.z(), p1.z(), p1.y(), ptr, alpha_mask, bump_tex, 
                                     ObjectToWorld, WorldToObject, false));
  list.add(std::make_shared<xz_rect>(p0.x(), p1.x(), p0.z(), p1.z(), p0.y(), ptr, alpha_mask, bump_tex, 
                                     ObjectToWorld, WorldToObject, true));
  list.add(std::make_shared<yz_rect>(p0.y(), p1.y(), p0.z(), p1.z(), p1.x(), ptr, alpha_mask, bump_tex, 
                                     ObjectToWorld, WorldToObject, false));
  list.add(std::make_shared<yz_rect>(p0.y(), p1.y(), p0.z(), p1.z(), p0.x(), ptr, alpha_mask, bump_tex, 
                                     ObjectToWorld, WorldToObject, true));
}

bool box::hit(const ray& r, Float t_min, Float t_max, hit_record& rec, random_gen& rng) {
  return(list.hit(r,t_min,t_max,rec, rng));
}

bool box::hit(const ray& r, Float t_min, Float t_max, hit_record& rec, Sampler* sampler) {
  return(list.hit(r,t_min,t_max,rec, sampler));
}
