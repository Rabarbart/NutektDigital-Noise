#include "unit_genericfx.h"
// Experimental identifiers: BART is the user's development prefix; confirm
// official registration before distributing outside the user's test devices.
const __unit_header genericfx_unit_header_t unit_header={
 .common={
  .header_size=sizeof(genericfx_unit_header_t),
  .target=UNIT_TARGET_PLATFORM|k_unit_module_genericfx,
  .api=UNIT_API_VERSION,
  .dev_id=0x42415254U,  // BART
  .unit_id=0x4E4C3230U, // NL20, distinct from NOISELAB 1.0
  .version=0x00020000U,
  .name="NOISELAB2",
  .num_params=8,
  .params={
    {0,7,0,0,k_unit_param_type_strings,0,0,0,{"TYPE"}},
    {0,1023,0,0,k_unit_param_type_none,0,0,0,{"ATTACK"}},
    {0,1023,0,300,k_unit_param_type_none,0,0,0,{"DECAY"}},
    {0,1023,0,350,k_unit_param_type_none,0,0,0,{"RELEASE"}},
    {0,1023,512,512,k_unit_param_type_none,0,0,0,{"TONE"}},
    {0,1023,0,140,k_unit_param_type_none,0,0,0,{"RESONANCE"}},
    {0,1023,0,0,k_unit_param_type_none,0,0,0,{"TEXTURE"}},
    {0,1023,0,650,k_unit_param_type_none,0,0,0,{"LEVEL"}}
  }
 },
 .default_mappings={
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,7,0},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,0},
    {k_genericfx_param_assign_y,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,300},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,350},
    {k_genericfx_param_assign_x,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,512},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,140},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,0},
    {k_genericfx_param_assign_depth,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1023,650}
 }
};
