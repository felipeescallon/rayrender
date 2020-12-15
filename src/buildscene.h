#ifndef BUILDSCENEH
#define BUILDSCENEH

#include "hitable.h"
#include "sphere.h"
#include "hitablelist.h"
#include "bvh_node.h"
#include "perlin.h"
#include "texture.h"
#include "rectangle.h"
#include "box.h"
#include "constant.h"
#include "triangle.h"
#include "pdf.h"
#include "trimesh.h"
#include "disk.h"
#include "cylinder.h"
#include "ellipsoid.h"
#include "cone.h"
#include "curve.h"
#include "csg.h"
#include "plymesh.h"
#include "mesh3d.h"
#include <Rcpp.h>
#include <memory>
using namespace Rcpp;


std::shared_ptr<hitable> rotation_order(std::shared_ptr<hitable> entry, NumericVector temprotvec, NumericVector order_rotation) {
  for(int i = 0; i < 3; i++) {
    if(order_rotation(i) == 1) {
      if(temprotvec(0) != 0) {
        entry = std::make_shared<rotate_x>(entry,temprotvec(0));
      }
    }
    if(order_rotation(i) == 2) {
      if(temprotvec(1) != 0) {
        entry = std::make_shared<rotate_y>(entry,temprotvec(1));
      }
    }
    if(order_rotation(i) == 3) {
      if(temprotvec(2) != 0) {
        entry = std::make_shared<rotate_z>(entry,temprotvec(2));
      }
    }
  }
  return(entry);
}


std::shared_ptr<hitable> build_scene(IntegerVector& type, 
                     NumericVector& radius, IntegerVector& shape,
                     List& position_list,
                     List& properties, List& velocity, LogicalVector& moving,
                     int n, Float shutteropen, Float shutterclose,
                     LogicalVector& ischeckered, List& checkercolors, 
                     List gradient_info,
                     NumericVector& noise, LogicalVector& isnoise,
                     NumericVector& noisephase, NumericVector& noiseintensity, List noisecolorlist,
                     List& angle, 
                     LogicalVector& isimage, LogicalVector has_alpha,
                     std::vector<Float* >& alpha_textures, std::vector<int* >& nveca,
                     std::vector<Float* >& textures, std::vector<int* >& nvec,
                     LogicalVector has_bump,
                     std::vector<Float* >& bump_textures, std::vector<int* >& nvecb,
                     NumericVector& bump_intensity,
                     NumericVector& lightintensity,
                     LogicalVector& isflipped,
                     LogicalVector& isvolume, NumericVector& voldensity,
                     List& order_rotation_list, 
                     LogicalVector& isgrouped, List& group_pivot, List& group_translate,
                     List& group_angle, List& group_order_rotation, List& group_scale,
                     LogicalVector& tri_normal_bools, LogicalVector& is_tri_color, List& tri_color_vert, 
                     CharacterVector& fileinfo, CharacterVector& filebasedir,
                     List& scale_list, NumericVector& sigma,  List &glossyinfo,
                     IntegerVector& shared_id_mat, LogicalVector& is_shared_mat,
                     std::vector<material* >* shared_materials, List& image_repeat_list,
                     List& csg_info, List& mesh_list, int bvh_type,
                     random_gen& rng) {
  hitable_list list;
  LogicalVector isgradient = gradient_info["isgradient"];
  List gradient_colors = gradient_info["gradient_colors"];
  LogicalVector gradient_trans = gradient_info["gradient_trans"];
  List gradient_control_points = gradient_info["gradient_control_points"];
  LogicalVector is_world_gradient = gradient_info["is_world_gradient"];
  LogicalVector gradient_is_hsv = gradient_info["type"];
  
  NumericVector x = position_list["xvec"];
  NumericVector y = position_list["yvec"];
  NumericVector z = position_list["zvec"];
  
  List csg_list = csg_info["csg"];

  NumericVector tempvector;
  NumericVector tempchecker;
  NumericVector tempgradient;
  NumericVector tempvel;
  NumericVector tempnoisecolor;
  NumericVector temprotvec;
  NumericVector order_rotation;
  NumericVector temp_gpivot;
  NumericVector temp_gtrans;
  NumericVector temp_gorder;
  NumericVector temp_gangle;
  NumericVector temp_gscale;
  NumericVector temp_tri_color;
  NumericVector temp_scales;
  NumericVector temp_glossy;
  NumericVector temp_repeat;
  NumericVector temp_gradient_control;
  vec3 gpivot;
  vec3 gtrans;
  vec3 gorder;
  vec3 gangle;
  int prop_len;
  
  List templist;
  vec3 center(x(0), y(0), z(0));
  vec3 vel(x(0), y(0), z(0));
  for(int i = 0; i < n; i++) {
    tempvector = as<NumericVector>(properties(i));
    tempgradient = as<NumericVector>(gradient_colors(i));
    tempchecker = as<NumericVector>(checkercolors(i));
    tempvel = as<NumericVector>(velocity(i));
    tempnoisecolor = as<NumericVector>(noisecolorlist(i));
    temprotvec = as<NumericVector>(angle(i));
    temp_tri_color = as<NumericVector>(tri_color_vert(i));
    order_rotation = as<NumericVector>(order_rotation_list(i));
    temp_scales = as<NumericVector>(scale_list(i));
    temp_glossy = as<NumericVector>(glossyinfo(i));
    temp_repeat = as<NumericVector>(image_repeat_list(i));
    temp_gradient_control = as<NumericVector>(gradient_control_points(i));

    bool is_scaled = false;
    bool is_group_scaled = false;
    if(temp_scales[0] != 1 || temp_scales[1] != 1 || temp_scales[2] != 1) {
      is_scaled = true;
    }
    if(isgrouped(i)) {
      temp_gpivot = as<NumericVector>(group_pivot(i));
      temp_gangle = as<NumericVector>(group_angle(i));
      temp_gorder = as<NumericVector>(group_order_rotation(i));
      temp_gtrans = as<NumericVector>(group_translate(i));
      temp_gscale = as<NumericVector>(group_scale(i));
      if(temp_gscale[0] != 1 || temp_gscale[1] != 1 || temp_gscale[2] != 1) {
        is_group_scaled = true;
      }
      gpivot = vec3(temp_gpivot(0),temp_gpivot(1),temp_gpivot(2));
      gtrans = vec3(temp_gtrans(0),temp_gtrans(1),temp_gtrans(2));
    } else {
      gpivot = vec3(0,0,0); 
      gtrans = vec3(0,0,0); 
    }
    prop_len=2;
    vel = vec3(tempvel(0),tempvel(1),tempvel(2));
    //Generate texture
    material *tex = nullptr;
    alpha_texture *alpha = nullptr;
    bump_texture *bump = nullptr;
    if(has_alpha(i)) {
      alpha = new alpha_texture(alpha_textures[i], nveca[i][0], nveca[i][1], nveca[i][2]);
    }
    if(has_bump(i)) {
      bump = new bump_texture(bump_textures[i], nvecb[i][0], nvecb[i][1], nvecb[i][2], 
                              bump_intensity(i));
    }
    if(type(i) == 2) {
      prop_len = 3;
    } else if (type(i) == 3) {
      prop_len = 7;
    } else if (type(i) == 8) {
      prop_len = 7;
    } else if (type(i) == 9) {
      prop_len = 6;
    }
    if(is_shared_mat(i) && shared_materials->size() > static_cast<size_t>(shared_id_mat(i))) {
      tex = shared_materials->at(shared_id_mat(i));
    } else {
      if(type(i) == 1) {
        if(isimage(i)) {
          tex = new lambertian(new image_texture(textures[i],nvec[i][0],nvec[i][1],nvec[i][2], 
                                                 temp_repeat[0], temp_repeat[1], 1.0));
        } else if (isnoise(i)) {
          tex = new lambertian(new noise_texture(noise(i),vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                 vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                                 noisephase(i), noiseintensity(i)));
        } else if (ischeckered(i)) {
          tex = new lambertian(new checker_texture(new constant_texture(vec3(tempchecker(0),tempchecker(1),tempchecker(2))),
                                                   new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),
                                                   tempchecker(3)));
        } else if (isgradient(i) && !is_world_gradient(i)) {
          tex = new lambertian(new gradient_texture(vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                    vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                    gradient_trans(i), gradient_is_hsv(i)));
        } else if (is_tri_color(i)) {
          tex = new lambertian(new triangle_texture(vec3(temp_tri_color(0),temp_tri_color(1),temp_tri_color(2)),
                                                    vec3(temp_tri_color(3),temp_tri_color(4),temp_tri_color(5)),
                                                    vec3(temp_tri_color(6),temp_tri_color(7),temp_tri_color(8))));
        } else if (is_world_gradient(i)) {
          tex = new lambertian(new world_gradient_texture(vec3(temp_gradient_control(0),temp_gradient_control(1),temp_gradient_control(2)),
                                                          vec3(temp_gradient_control(3),temp_gradient_control(4),temp_gradient_control(5)),
                                                          vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                          vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                          gradient_is_hsv(i)));
        } else {
          tex = new lambertian(new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))));
        }
      } else if (type(i) == 2) {
        if(isimage(i)) {
          tex = new metal(new image_texture(textures[i],nvec[i][0],nvec[i][1],nvec[i][2], 
                                            temp_repeat[0], temp_repeat[1], 1.0),
                          tempvector(3), 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (isnoise(i)) {
          tex = new metal(new noise_texture(noise(i),vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                 vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                                 noisephase(i), noiseintensity(i)),
                          tempvector(3), 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (ischeckered(i)) {
          tex = new metal(new checker_texture(new constant_texture(vec3(tempchecker(0),tempchecker(1),tempchecker(2))),
                                              new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),
                                              tempchecker(3)),
                          tempvector(3), 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (isgradient(i) && !is_world_gradient(i)) {
          tex = new metal(new gradient_texture(vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                    vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                    gradient_trans(i), gradient_is_hsv(i)),
                          tempvector(3), 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (is_tri_color(i)) {
          tex = new metal(new triangle_texture(vec3(temp_tri_color(0),temp_tri_color(1),temp_tri_color(2)),
                                                    vec3(temp_tri_color(3),temp_tri_color(4),temp_tri_color(5)),
                                                    vec3(temp_tri_color(6),temp_tri_color(7),temp_tri_color(8))),
                          tempvector(3), 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (is_world_gradient(i)) {
          tex = new metal(new world_gradient_texture(vec3(temp_gradient_control(0),temp_gradient_control(1),temp_gradient_control(2)),
                                                     vec3(temp_gradient_control(3),temp_gradient_control(4),temp_gradient_control(5)),
                                                     vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                     vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                     gradient_is_hsv(i)),
                         tempvector(3), 
                         vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                         vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else {
          tex = new metal(new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),
                          tempvector(3), 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        }
      } else if (type(i) == 3) {
        tex = new dielectric(vec3(tempvector(0),tempvector(1),tempvector(2)), tempvector(3), 
                             vec3(tempvector(4),tempvector(5),tempvector(6)), 
                             tempvector(7), rng);
      } else if (type(i) == 4) {
        if(isimage(i)) {
          tex = new orennayar(new image_texture(textures[i],nvec[i][0],nvec[i][1],nvec[i][2], 
                                                temp_repeat[0], temp_repeat[1], 1.0), sigma(i));
        } else if (isnoise(i)) {
          tex = new orennayar(new noise_texture(noise(i),vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                 vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                                 noisephase(i), noiseintensity(i)), sigma(i));
        } else if (ischeckered(i)) {
          tex = new orennayar(new checker_texture(new constant_texture(vec3(tempchecker(0),tempchecker(1),tempchecker(2))),
                                                   new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),tempchecker(3)), 
                                                   sigma(i));
        } else if (isgradient(i) && !is_world_gradient(i)) {
          tex = new orennayar(new gradient_texture(vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                    vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                    gradient_trans(i), gradient_is_hsv(i)), sigma(i));
        } else if (is_tri_color(i)) {
          tex = new orennayar(new triangle_texture(vec3(temp_tri_color(0),temp_tri_color(1),temp_tri_color(2)),
                                                    vec3(temp_tri_color(3),temp_tri_color(4),temp_tri_color(5)),
                                                    vec3(temp_tri_color(6),temp_tri_color(7),temp_tri_color(8))), sigma(i) );
        } else if (is_world_gradient(i)) {
          tex = new orennayar(new world_gradient_texture(vec3(temp_gradient_control(0),temp_gradient_control(1),temp_gradient_control(2)),
                                                         vec3(temp_gradient_control(3),temp_gradient_control(4),temp_gradient_control(5)),
                                                         vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                         vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                         gradient_is_hsv(i)), sigma(i));
        } else {
          tex = new orennayar(new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))), sigma(i)); //marked as small definite loss in valgrind memcheck
        }
      } else if (type(i) == 5) {
        texture* light_tex;
        if(isimage(i)) {
          light_tex = new image_texture(textures[i],nvec[i][0],nvec[i][1],nvec[i][2], 
                                        temp_repeat[0], temp_repeat[1], 1.0);
        } else if (isnoise(i)) {
          light_tex = new noise_texture(noise(i),vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                                noisephase(i), noiseintensity(i));
        } else if (ischeckered(i)) {
          light_tex = new checker_texture(new constant_texture(vec3(tempchecker(0),tempchecker(1),tempchecker(2))),
                                                  new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),tempchecker(3));
        } else if (isgradient(i) && !is_world_gradient(i)) {
          light_tex = new gradient_texture(vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                   vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                   gradient_trans(i), gradient_is_hsv(i));
        } else if (is_tri_color(i)) {
          light_tex = new triangle_texture(vec3(temp_tri_color(0),temp_tri_color(1),temp_tri_color(2)),
                                                   vec3(temp_tri_color(3),temp_tri_color(4),temp_tri_color(5)),
                                                   vec3(temp_tri_color(6),temp_tri_color(7),temp_tri_color(8)));
        } else if (is_world_gradient(i)) {
          light_tex = new world_gradient_texture(vec3(temp_gradient_control(0),temp_gradient_control(1),temp_gradient_control(2)),
                                                         vec3(temp_gradient_control(3),temp_gradient_control(4),temp_gradient_control(5)),
                                                         vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                         vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                         gradient_is_hsv(i));
        } else {
          light_tex = new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)));
        }
        tex = new diffuse_light(light_tex, lightintensity(i));
      } else if (type(i) == 6) {
        MicrofacetDistribution *dist;
        if(temp_glossy(0) == 1) {
          dist = new TrowbridgeReitzDistribution(temp_glossy(1), temp_glossy(2), true, true);
        } else {
          dist = new BeckmannDistribution(temp_glossy(1), temp_glossy(2), false, true);
        }
        if(isimage(i)) {
          tex = new MicrofacetReflection(new image_texture(textures[i],nvec[i][0],nvec[i][1],nvec[i][2], 
                                                           temp_repeat[0], temp_repeat[1], 1.0), dist, 
                                         vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                         vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (isnoise(i)) {
          tex = new MicrofacetReflection(new noise_texture(noise(i),vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                                noisephase(i), noiseintensity(i)), dist, 
                                                vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                                vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (ischeckered(i)) {
          tex = new MicrofacetReflection(new checker_texture(new constant_texture(vec3(tempchecker(0),tempchecker(1),tempchecker(2))),
                                                  new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),tempchecker(3)), 
                                                  dist, 
                                                  vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                                  vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        }  else if (isgradient(i) && !is_world_gradient(i)) {
          tex = new MicrofacetReflection(new gradient_texture(vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                   vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                   gradient_trans(i), gradient_is_hsv(i)), dist, 
                                                   vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                                   vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (is_tri_color(i)) {
          tex = new MicrofacetReflection(new triangle_texture(vec3(temp_tri_color(0),temp_tri_color(1),temp_tri_color(2)),
                                                   vec3(temp_tri_color(3),temp_tri_color(4),temp_tri_color(5)),
                                                   vec3(temp_tri_color(6),temp_tri_color(7),temp_tri_color(8))), dist, 
                                                   vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                                   vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (is_world_gradient(i)) {
          tex = new MicrofacetReflection(new world_gradient_texture(vec3(temp_gradient_control(0),temp_gradient_control(1),temp_gradient_control(2)),
                                                                    vec3(temp_gradient_control(3),temp_gradient_control(4),temp_gradient_control(5)),
                                                                    vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                                    vec3(tempgradient(0),tempgradient(1),tempgradient(2)), gradient_is_hsv(i)), dist, 
                                                                    vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                                                    vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else {
          tex = new MicrofacetReflection(new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))), dist, 
                                         vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                                         vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        }
      } if (type(i) == 7) {
        MicrofacetDistribution *dist;
        if(temp_glossy(0) == 1) {
          dist = new TrowbridgeReitzDistribution(temp_glossy(1), temp_glossy(2), true, true);
        } else {
          dist = new BeckmannDistribution(temp_glossy(1), temp_glossy(2), false, true);
        }
        if(isimage(i)) {
          tex = new glossy(new image_texture(textures[i],nvec[i][0],nvec[i][1],nvec[i][2], 
                                             temp_repeat[0], temp_repeat[1], 1.0), dist, 
                           vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                           vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (isnoise(i)) {
          tex = new glossy(new noise_texture(noise(i),vec3(tempvector(0),tempvector(1),tempvector(2)),
                                             vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                             noisephase(i), noiseintensity(i)), dist, 
                           vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                           vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (ischeckered(i)) {
          tex = new glossy(new checker_texture(new constant_texture(vec3(tempchecker(0),tempchecker(1),tempchecker(2))),
                                               new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))),tempchecker(3)), 
                           dist, 
                           vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                           vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        }  else if (isgradient(i) && !is_world_gradient(i)) {
          tex = new glossy(new gradient_texture(vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                     vec3(tempgradient(0),tempgradient(1),tempgradient(2)),
                                                     gradient_trans(i), gradient_is_hsv(i)), dist, 
                           vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                           vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (is_tri_color(i)) {
          tex = new glossy(new triangle_texture(vec3(temp_tri_color(0),temp_tri_color(1),temp_tri_color(2)),
                                                vec3(temp_tri_color(3),temp_tri_color(4),temp_tri_color(5)),
                                                vec3(temp_tri_color(6),temp_tri_color(7),temp_tri_color(8))), dist, 
                           vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                           vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else if (is_world_gradient(i)) {
          tex = new glossy(new world_gradient_texture(vec3(temp_gradient_control(0),temp_gradient_control(1),temp_gradient_control(2)),
                                                      vec3(temp_gradient_control(3),temp_gradient_control(4),temp_gradient_control(5)),
                                                      vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                      vec3(tempgradient(0),tempgradient(1),tempgradient(2)), gradient_is_hsv(i)), dist, 
                          vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                          vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        } else {
          tex = new glossy(new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))), dist, 
                           vec3(temp_glossy(3), temp_glossy(4), temp_glossy(5)), 
                           vec3(temp_glossy(6),temp_glossy(7),temp_glossy(8)));
        }
      } else if (type(i) == 8) {
        tex = new spot_light(new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2))*lightintensity(i)),
                             vec3(tempvector(3),tempvector(4),tempvector(5)), tempvector(6),tempvector(7));
      } else if (type(i) == 9) {
        tex = new hair(vec3(tempvector(0),tempvector(1),tempvector(2)), 
                       tempvector(3),tempvector(4),tempvector(5),tempvector(6));
      }
    }
    if(is_shared_mat(i) && shared_materials->size() <= static_cast<size_t>(shared_id_mat(i)) ) {
      shared_materials->push_back(tex);
    }
    //Generate center vector
    if(shape(i) == 1) {
      center =  vec3(x(i), y(i), z(i));
    } else if(shape(i) == 2) {
      center =  vec3(tempvector(prop_len+1),tempvector(prop_len+3),tempvector(prop_len+5));
    } else if(shape(i) == 3) {
      center =  vec3(tempvector(prop_len+1),tempvector(prop_len+5), tempvector(prop_len+3));
    } else if(shape(i) == 4) {
      center =  vec3(tempvector(prop_len+5),tempvector(prop_len+1),tempvector(prop_len+3));
    } else if(shape(i) == 5) {
      center =  vec3(x(i), y(i), z(i));
    } else if(shape(i) == 6) {
      center =  vec3(0, 0, 0);
    } else if(shape(i) == 7) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 8) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 9) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 10) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 11) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 12) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 13) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 14) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 15) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 16) {
      center = vec3(x(i), y(i), z(i));
    } else if(shape(i) == 17) {
      center = vec3(x(i), y(i), z(i));
    }

    //Generate objects
    if (shape(i) == 1) {
      
      //Fix moving memory leak
      std::shared_ptr<hitable> entry = std::make_shared<sphere>(vec3(0,0,0), radius(i), tex, alpha, bump);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      if(!moving(i)) {
        entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      } else {
        entry = std::make_shared<moving_sphere>(center + vel * shutteropen, 
                                  center + vel * shutterclose, 
                                  shutteropen, shutterclose, radius(i), tex, alpha, bump);
      }
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      if(isvolume(i)) {
        list.add(std::make_shared<constant_medium>(entry, voldensity(i), new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));
      } else {
        list.add(entry);;
      }
    } else if (shape(i)  == 2) {
      std::shared_ptr<hitable> entry = std::make_shared<xy_rect>(-tempvector(prop_len+2)/2,tempvector(prop_len+2)/2,
                                   -tempvector(prop_len+4)/2,tempvector(prop_len+4)/2,
                                   0, tex, alpha, bump, isflipped(i));
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry,center + gtrans + vel * shutteropen);
      list.add(entry);;
    } else if (shape(i)  == 3) {
      std::shared_ptr<hitable> entry = std::make_shared<xz_rect>(-tempvector(prop_len+2)/2,tempvector(prop_len+2)/2,
                                   -tempvector(prop_len+4)/2,tempvector(prop_len+4)/2,
                                   0, tex, alpha, bump, isflipped(i));
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry,center + gtrans + vel * shutteropen);
      list.add(entry);;
    } else if (shape(i)  == 4) {
      std::shared_ptr<hitable> entry = std::make_shared<yz_rect>(-tempvector(prop_len+2)/2,tempvector(prop_len+2)/2,
                                   -tempvector(prop_len+4)/2,tempvector(prop_len+4)/2,
                                   0, tex, alpha, bump, isflipped(i));
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry,center + gtrans + vel * shutteropen);
      list.add(entry);;
    } else if (shape(i)  == 5) {
      std::shared_ptr<hitable> entry = std::make_shared<box>(-vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3))/2, 
                               vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3))/2, 
                               tex, alpha, bump);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isvolume(i)) {
        if(!isnoise(i)) {
          list.add(std::make_shared<constant_medium>(entry,voldensity(i), new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));;
        } else {
          list.add(std::make_shared<constant_medium>(entry,voldensity(i), 
                                        new noise_texture(noise(i),
                                                          vec3(tempvector(0),tempvector(1),tempvector(2)),
                                                          vec3(tempnoisecolor(0),tempnoisecolor(1),tempnoisecolor(2)),
                                                          noisephase(i), noiseintensity(i))));
        }
      } else {
        list.add(entry);;
      }
    } else if (shape(i)  == 6) {
      std::shared_ptr<hitable> entry;
      if(tri_normal_bools(i)) {
        entry= std::make_shared<triangle>(vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3)),
                            vec3(tempvector(prop_len+4),tempvector(prop_len+5),tempvector(prop_len+6)),
                            vec3(tempvector(prop_len+7),tempvector(prop_len+8),tempvector(prop_len+9)),
                            vec3(tempvector(prop_len+10),tempvector(prop_len+11),tempvector(prop_len+12)),
                            vec3(tempvector(prop_len+13),tempvector(prop_len+14),tempvector(prop_len+15)),
                            vec3(tempvector(prop_len+16),tempvector(prop_len+17),tempvector(prop_len+18)),
                            !is_shared_mat(i), //turn off if shared material (e.g. extruded polygon)
                            tex, alpha, bump);
      } else {
        entry= std::make_shared<triangle>(vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3)),
                            vec3(tempvector(prop_len+4),tempvector(prop_len+5),tempvector(prop_len+6)),
                            vec3(tempvector(prop_len+7),tempvector(prop_len+8),tempvector(prop_len+9)),
                            !is_shared_mat(i), //turn off if shared material (e.g. extruded polygon)
                            tex, alpha, bump);
      }
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        list.add(std::make_shared<flip_normals>(entry));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 7) {
      std::shared_ptr<hitable> entry;
      std::string objfilename = Rcpp::as<std::string>(fileinfo(i));
      std::string objbasedirname = Rcpp::as<std::string>(filebasedir(i));
      entry = std::make_shared<trimesh>(objfilename, objbasedirname, 
                         tex,
                         tempvector(prop_len+1),
                         shutteropen, shutterclose, bvh_type, rng);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      } 
      if(isvolume(i)) {
        list.add(std::make_shared<constant_medium>(entry, voldensity(i), 
                                      new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 8) {
      std::shared_ptr<hitable> entry;
      std::string objfilename = Rcpp::as<std::string>(fileinfo(i));
      std::string objbasedirname = Rcpp::as<std::string>(filebasedir(i));
      if(sigma(i) == 0) {
        entry = std::make_shared<trimesh>(objfilename, objbasedirname, 
                            tempvector(prop_len+1), 
                            shutteropen, shutterclose, bvh_type, rng);
      } else {
        entry = std::make_shared<trimesh>(objfilename, objbasedirname, 
                            tempvector(prop_len+1), sigma(i),
                            shutteropen, shutterclose, bvh_type, rng);
      }
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      } 
      if(isvolume(i)) {
        list.add(std::make_shared<constant_medium>(entry, voldensity(i), 
                                      new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 9) {
      std::shared_ptr<hitable> entry;
      entry = std::make_shared<disk>(vec3(0,0,0), radius(i), tempvector(prop_len+1), tex, alpha, bump);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        list.add(std::make_shared<flip_normals>(entry));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 10) {
      bool has_caps = type(i) != 5 && type(i) != 8;
      std::shared_ptr<hitable> entry = std::make_shared<cylinder>(radius(i), tempvector(prop_len+1), 
                                    tempvector(prop_len+2), tempvector(prop_len+3),has_caps,
                                    tex, alpha, bump);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      list.add(entry);
    } else if (shape(i) == 11) {
      std::shared_ptr<hitable> entry = std::make_shared<ellipsoid>(vec3(0,0,0), radius(i), 
                                     vec3(tempvector(prop_len + 1), tempvector(prop_len + 2), tempvector(prop_len + 3)),
                                     tex, alpha, bump);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      if(isvolume(i)) {
        list.add(std::make_shared<constant_medium>(entry, voldensity(i), 
                                      new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 12) {
      std::shared_ptr<hitable> entry;
      std::string objfilename = Rcpp::as<std::string>(fileinfo(i));
      std::string objbasedirname = Rcpp::as<std::string>(filebasedir(i));
      entry = std::make_shared<trimesh>(objfilename, objbasedirname, 
                          sigma(i),
                          tempvector(prop_len+1), true,
                          shutteropen, shutterclose, bvh_type, rng);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      } 
      if(isvolume(i)) {
        list.add(std::make_shared<constant_medium>(entry, voldensity(i), 
                                      new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 13) {
      std::shared_ptr<hitable> entry = std::make_shared<cone>(radius(i), tempvector(prop_len+1), tex, alpha, bump);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      list.add(entry);
    } else if (shape(i) == 14) {
      int pl = prop_len;
      vec3 p[4];
      vec3 n[2];
      CurveType type_curve;
      if(tempvector(pl+17) == 1) {
        type_curve = CurveType::Flat;
      } else if (tempvector(pl+17) == 2) {
        type_curve = CurveType::Cylinder;
      } else {
        type_curve = CurveType::Ribbon;
      }
      p[0] = vec3(tempvector(pl+1),tempvector(pl+2),tempvector(pl+3));
      p[1] = vec3(tempvector(pl+4),tempvector(pl+5),tempvector(pl+6));
      p[2] = vec3(tempvector(pl+7),tempvector(pl+8),tempvector(pl+9));
      p[3] = vec3(tempvector(pl+10),tempvector(pl+11),tempvector(pl+12));
      
      n[0] = vec3(tempvector(pl+18), tempvector(pl+19), tempvector(pl+20));
      n[1] = vec3(tempvector(pl+21), tempvector(pl+22), tempvector(pl+23));
      
      std::shared_ptr<CurveCommon> curve_data = std::make_shared<CurveCommon>(p, tempvector(pl+13), tempvector(pl+14), type_curve, n);
      std::shared_ptr<hitable> entry = std::make_shared<curve>(tempvector(pl+15),tempvector(pl+16), curve_data, tex);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      list.add(entry);
    } else if (shape(i) == 15) {
      List temp_csg = csg_list(i);
      std::shared_ptr<ImplicitShape> shapes = parse_csg(temp_csg);
      std::shared_ptr<hitable> entry = std::make_shared<csg>(tex, shapes);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      list.add(entry);
    } else if (shape(i) == 16) {
      std::shared_ptr<hitable> entry;
      std::string objfilename = Rcpp::as<std::string>(fileinfo(i));
      std::string objbasedirname = Rcpp::as<std::string>(filebasedir(i));
      entry = std::make_shared<plymesh>(objfilename, objbasedirname, 
                          tex,
                          tempvector(prop_len+1),
                          shutteropen, shutterclose, bvh_type, rng);
      if(entry == nullptr) {
        continue;
      }
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      } 
      if(isvolume(i)) {
        list.add(std::make_shared<constant_medium>(entry, voldensity(i), 
                                      new constant_texture(vec3(tempvector(0),tempvector(1),tempvector(2)))));
      } else {
        list.add(entry);
      }
    } else if (shape(i) == 17) {
      List mesh_entry = mesh_list(i);
      std::shared_ptr<hitable> entry = std::make_shared<mesh3d>(mesh_entry, tex,
                                  shutteropen, shutterclose, bvh_type, rng);
      if(is_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
      }
      entry = rotation_order(entry, temprotvec, order_rotation);
      if(isgrouped(i)) {
        entry = std::make_shared<translate>(entry, center - gpivot);
        if(is_group_scaled) {
          entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
        }
        entry = rotation_order(entry, temp_gangle, temp_gorder);
        entry = std::make_shared<translate>(entry, -center + gpivot );
      }
      entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
      if(isflipped(i)) {
        entry = std::make_shared<flip_normals>(entry);
      }
      list.add(entry);
    }
  }
  std::shared_ptr<hitable> full_scene = std::make_shared<bvh_node>(list, shutteropen, shutterclose, bvh_type, rng);
  return(full_scene);
}

std::shared_ptr<hitable> build_imp_sample(IntegerVector& type, 
                          NumericVector& radius, IntegerVector& shape,
                          List& position_list,
                          List& properties, List& velocity, 
                          int n, Float shutteropen, Float shutterclose, 
                          List& angle, int i, List& order_rotation_list,
                          LogicalVector& isgrouped, 
                          List& group_pivot, List& group_translate,
                          List& group_angle, List& group_order_rotation, List& group_scale,
                          CharacterVector& fileinfo, CharacterVector& filebasedir,
                          List& scale_list, List& mesh_list, int bvh_type, random_gen& rng) {
  NumericVector x = position_list["xvec"];
  NumericVector y = position_list["yvec"];
  NumericVector z = position_list["zvec"];
  
  NumericVector tempvector;
  NumericVector temprotvec;
  NumericVector tempvel;
  NumericVector order_rotation;
  NumericVector temp_gpivot;
  NumericVector temp_gtrans;
  NumericVector temp_gorder;
  NumericVector temp_gangle;
  NumericVector temp_gscale;
  NumericVector temp_scales;
  vec3 gpivot;
  vec3 gtrans;
  vec3 gorder;
  vec3 gangle;
  
  List templist;
  int prop_len;
  tempvector = as<NumericVector>(properties(i));
  tempvel = as<NumericVector>(velocity(i));
  temprotvec =  as<NumericVector>(angle(i));
  order_rotation = as<NumericVector>(order_rotation_list(i));
  temp_scales = as<NumericVector>(scale_list(i));
  
  vec3 center(x(i), y(i), z(i));
  vec3 vel(tempvel(0),tempvel(1),tempvel(2));
  bool is_scaled = false;
  bool is_group_scaled = false;
  if(temp_scales[0] != 1 || temp_scales[1] != 1 || temp_scales[2] != 1) {
    is_scaled = true;
  }
  prop_len=2;
  if(isgrouped(i)) {
    temp_gpivot = as<NumericVector>(group_pivot(i));
    temp_gangle = as<NumericVector>(group_angle(i));
    temp_gorder = as<NumericVector>(group_order_rotation(i));
    temp_gtrans = as<NumericVector>(group_translate(i));
    temp_gscale = as<NumericVector>(group_scale(i));
    if(temp_gscale[0] != 1 || temp_gscale[1] != 1 || temp_gscale[2] != 1) {
      is_group_scaled = true;
    }
    gpivot = vec3(temp_gpivot(0),temp_gpivot(1),temp_gpivot(2));
    gtrans = vec3(temp_gtrans(0),temp_gtrans(1),temp_gtrans(2));
  } else{
    gpivot = vec3(0,0,0); 
    gtrans = vec3(0,0,0); 
  }
  if(type(i) == 3) {
    prop_len = 7;
  } else if(type(i) == 2) {
    prop_len = 3;
  } else if (type(i) == 8) {
    prop_len = 7;
  } else if (type(i) == 9) {
    prop_len = 6;
  }
  
  if(shape(i) == 1) {
    center =  vec3(x(i), y(i), z(i));
  } else if(shape(i) == 2) {
    center =  vec3(tempvector(prop_len+1),tempvector(prop_len+3),tempvector(prop_len+5));
  } else if(shape(i) == 3) {
    center =  vec3(tempvector(prop_len+1),tempvector(prop_len+5), tempvector(prop_len+3));
  } else if(shape(i) == 4) {
    center =  vec3(tempvector(prop_len+5),tempvector(prop_len+1),tempvector(prop_len+3));
  } else if(shape(i) == 5) {
    center =  vec3(x(i), y(i), z(i));
  } else if(shape(i) == 6) {
    center =  vec3(x(i), y(i), z(i));
  } else if(shape(i) == 7) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 8) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 9) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 10) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 11) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 12) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 13) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 14) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 15) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 16) {
    center = vec3(x(i), y(i), z(i));
  } else if(shape(i) == 17) {
    center = vec3(x(i), y(i), z(i));
  }
  
  material *tex = nullptr;
  alpha_texture *alpha = nullptr;
  bump_texture *bump = nullptr;

  if(shape(i) == 1) {
    std::shared_ptr<hitable> entry = std::make_shared<sphere>(vec3(0,0,0), radius(i), tex, alpha,bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 2) {
    std::shared_ptr<hitable> entry = std::make_shared<xy_rect>(-tempvector(prop_len+2)/2,tempvector(prop_len+2)/2,
                                 -tempvector(prop_len+4)/2,tempvector(prop_len+4)/2,
                                 0, tex, alpha, bump, false);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 3) {
    std::shared_ptr<hitable> entry = std::make_shared<xz_rect>(-tempvector(prop_len+2)/2,tempvector(prop_len+2)/2,
                                 -tempvector(prop_len+4)/2,tempvector(prop_len+4)/2,
                                 0, tex, alpha, bump, false);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 4) {
    std::shared_ptr<hitable> entry = std::make_shared<yz_rect>(-tempvector(prop_len+2)/2,tempvector(prop_len+2)/2,
                                 -tempvector(prop_len+4)/2,tempvector(prop_len+4)/2,
                                 0, tex, alpha, bump, false);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 5) {
    std::shared_ptr<hitable> entry = std::make_shared<box>(-vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3))/2, 
                             vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3))/2, 
                             tex, alpha,bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 6) {
    std::shared_ptr<hitable> entry = std::make_shared<triangle>(vec3(tempvector(prop_len+1),tempvector(prop_len+2),tempvector(prop_len+3)),
                                  vec3(tempvector(prop_len+4),tempvector(prop_len+5),tempvector(prop_len+6)),
                                  vec3(tempvector(prop_len+7),tempvector(prop_len+8),tempvector(prop_len+9)),
                                  false,
                                  tex, alpha, bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 7 || shape(i) == 8 || shape(i) == 12) {
    std::shared_ptr<hitable> entry;
    std::string objfilename = Rcpp::as<std::string>(fileinfo(i));
    std::string objbasedirname = Rcpp::as<std::string>(filebasedir(i));
    entry = std::make_shared<trimesh>(objfilename, objbasedirname,
                        tempvector(prop_len+1),
                        shutteropen, shutterclose, bvh_type, rng);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 9) {
    std::shared_ptr<hitable> entry;
    entry = std::make_shared<disk>(vec3(0,0,0), radius(i), tempvector(prop_len+1), tex, alpha,bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 10) {
    bool has_caps = type(i) != 5 && type(i) != 8;
    std::shared_ptr<hitable> entry = std::make_shared<cylinder>(radius(i), tempvector(prop_len+1), 
                                  tempvector(prop_len+2), tempvector(prop_len+3),has_caps, 
                                  tex, alpha,bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 11) {
    std::shared_ptr<hitable> entry = std::make_shared<ellipsoid>(vec3(0,0,0), radius(i), 
                                   vec3(tempvector(prop_len + 1), tempvector(prop_len + 2), tempvector(prop_len + 3)),
                                   tex, alpha,bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 13) {
    std::shared_ptr<hitable> entry = std::make_shared<cone>(radius(i), tempvector(prop_len+1), 
                                                            tex, alpha, bump);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 14) {
    int pl = prop_len;
    vec3 p[4];
    p[0] = vec3(tempvector(pl+1),tempvector(pl+2),tempvector(pl+3));
    p[1] = vec3(tempvector(pl+4),tempvector(pl+5),tempvector(pl+6));
    p[2] = vec3(tempvector(pl+7),tempvector(pl+8),tempvector(pl+9));
    p[3] = vec3(tempvector(pl+10),tempvector(pl+11),tempvector(pl+12));
    std::shared_ptr<CurveCommon> curve_data = std::make_shared<CurveCommon>(p, tempvector(pl+13), tempvector(pl+14), CurveType::Flat, nullptr);
    std::shared_ptr<hitable> entry = std::make_shared<curve>(tempvector(pl+15),tempvector(pl+16), curve_data, tex);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 15) {
    std::shared_ptr<ImplicitShape> shapes;
    std::shared_ptr<hitable> entry = std::make_shared<csg>(tex, shapes);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else if (shape(i) == 16) {
    std::shared_ptr<hitable> entry;
    std::string objfilename = Rcpp::as<std::string>(fileinfo(i));
    std::string objbasedirname = Rcpp::as<std::string>(filebasedir(i));
    entry = std::make_shared<plymesh>(objfilename, objbasedirname, 
                        tex,
                        tempvector(prop_len+1),
                        shutteropen, shutterclose, bvh_type, rng);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  } else {
    List mesh_entry = mesh_list(i);
    std::shared_ptr<hitable> entry = std::make_shared<mesh3d>(mesh_entry, tex,
                                shutteropen, shutterclose, bvh_type, rng);
    if(is_scaled) {
      entry = std::make_shared<scale>(entry, vec3(temp_scales[0], temp_scales[1], temp_scales[2]));
    }
    entry = rotation_order(entry, temprotvec, order_rotation);
    if(isgrouped(i)) {
      entry = std::make_shared<translate>(entry, center - gpivot);
      if(is_group_scaled) {
        entry = std::make_shared<scale>(entry, vec3(temp_gscale[0], temp_gscale[1], temp_gscale[2]));
      }
      entry = rotation_order(entry, temp_gangle, temp_gorder);
      entry = std::make_shared<translate>(entry, -center + gpivot );
    }
    entry = std::make_shared<translate>(entry, center + gtrans + vel * shutteropen);
    return(entry);
  }
}

#endif
