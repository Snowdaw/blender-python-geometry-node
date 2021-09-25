#pragma once

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file
 * \ingroup bke
 * \brief New brush engine for sculpt
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "RNA_types.h"

/*
The new brush engine is based on command lists.  These lists
will eventually be created by a node editor.

Key is the concept of BrushChannels.  A brush channel is
a logical parameter with a type, input settings (e.g. pen),
a falloff curve, etc.

Brush channels have a concept of inheritance.  There is a
BrushChannelSet (collection of channels) in Sculpt,
in Brush, and in BrushCommand.  Inheritence behavior
is controller via BrushChannel->flag.

This should completely replace UnifiedPaintSettings.
*/

#include "DNA_sculpt_brush_types.h"

struct BrushChannel;
struct BlendWriter;
struct BlendDataReader;
struct BlendLibReader;
struct ID;
struct BlendExpander;
struct Brush;
struct Sculpt;

#define MAKE_BUILTIN_CH_NAME(idname) BRUSH_BUILTIN_##idname

/* these macros check channel names at compile time */

#define BRUSHSET_LOOKUP(chset, channel) \
  BKE_brush_channelset_lookup(chset, MAKE_BUILTIN_CH_NAME(channel))
#define BRUSHSET_HAS(chset, channel, mapdata) \
  BKE_brush_channelset_lookup(chset, MAKE_BUILTIN_CH_NAME(channel))
#define BRUSHSET_GET_FLOAT(chset, channel, mapdata) \
  BKE_brush_channelset_get_float(chset, MAKE_BUILTIN_CH_NAME(channel), mapdata)
#define BRUSHSET_GET_FINAL_FLOAT(childset, parentset, channel, mapdata) \
  BKE_brush_channelset_get_final_float(childset, parentset, MAKE_BUILTIN_CH_NAME(channel), mapdata)
#define BRUSHSET_GET_INT(chset, channel, mapdata) \
  BKE_brush_channelset_get_int(chset, MAKE_BUILTIN_CH_NAME(channel), mapdata)
#define BRUSHSET_ENSURE_BUILTIN(chset, channel) \
  BKE_brush_channelset_ensure_builtin(chset, MAKE_BUILTIN_CH_NAME(channel))
#define BRUSHSET_SET_FLOAT(chset, channel, val) \
  BKE_brush_channelset_set_float(chset, MAKE_BUILTIN_CH_NAME(channel), val)
#define BRUSHSET_SET_INT(chset, channel, val) \
  BKE_brush_channelset_set_int(chset, MAKE_BUILTIN_CH_NAME(channel), val)

//#define DEBUG_CURVE_MAPPING_ALLOC
#ifdef DEBUG_CURVE_MAPPING_ALLOC
void namestack_push(const char *name);
void *namestack_pop(void *passthru);
#endif

typedef struct BrushMappingDef {
  int curve;
  bool enabled;
  bool inv;
  float min, max;
  int blendmode;
  float factor;  // if 0, will default to 1.0
} BrushMappingDef;

typedef struct BrushMappingPreset {
  // must match order of BRUSH_MAPPING_XXX enums
  struct BrushMappingDef pressure, xtilt, ytilt, angle, speed;
} BrushMappingPreset;

typedef struct BrushMappingData {
  float pressure, xtilt, ytilt, angle, speed;
} BrushMappingData;

#define MAX_BRUSH_ENUM_DEF 32

typedef struct BrushEnumDef {
  int value;
  const char identifier[64];
  char icon[32];  // don't forget when writing literals that icon here is a string, not an int!
  const char name[64];
  const char description[512];
} BrushEnumDef;

typedef struct BrushChannelType {
  char name[128], idname[64], tooltip[512];
  float min, max, soft_min, soft_max;
  BrushMappingPreset mappings;

  int type, flag;
  int ivalue;
  float fvalue;
  float vector[4];
  int curve_preset;

  BrushEnumDef enumdef[MAX_BRUSH_ENUM_DEF];  // for enum/bitmask types
  EnumPropertyItem *rna_enumdef;

  bool user_defined;
} BrushChannelType;

typedef struct BrushCommand {
  int tool;
  float last_spacing_t[512];  // for different symmetry passes
  struct BrushChannelSet *params;
  struct BrushChannelSet *params_final;
  struct BrushChannelSet *params_mapped;
} BrushCommand;

typedef struct BrushCommandList {
  BrushCommand *commands;
  int totcommand;
} BrushCommandList;

void BKE_brush_channel_free_data(BrushChannel *ch);
void BKE_brush_channel_free(BrushChannel *ch);
void BKE_brush_channel_copy_data(BrushChannel *dst, BrushChannel *src, bool keep_mappings);
void BKE_brush_channel_init(BrushChannel *ch, BrushChannelType *def);

BrushChannelSet *BKE_brush_channelset_create();
#ifdef DEBUG_CURVE_MAPPING_ALLOC
BrushChannelSet *_BKE_brush_channelset_copy(BrushChannelSet *src);
#  define BKE_brush_channelset_copy(src) \
    (namestack_push(__func__), (BrushChannelSet *)namestack_pop(_BKE_brush_channelset_copy(src)))
#else
BrushChannelSet *BKE_brush_channelset_copy(BrushChannelSet *src);
#endif
void BKE_brush_channelset_free(BrushChannelSet *chset);

void BKE_brush_channelset_add(BrushChannelSet *chset, BrushChannel *ch);

/* makes a copy of ch and adds it to the channel set */
void BKE_brush_channelset_add_duplicate(BrushChannelSet *chset, BrushChannel *ch);

/* finds a unique name for ch, does not add it to chset->namemap */
void BKE_brush_channel_ensure_unque_name(BrushChannelSet *chset, BrushChannel *ch);

/* does not free ch or its data */
void BKE_brush_channelset_remove(BrushChannelSet *chset, BrushChannel *ch);

// does not free ch or its data
bool BKE_brush_channelset_remove_named(BrushChannelSet *chset, const char *idname);

// checks is a channel with existing->idname exists; if not a copy of existing is made and inserted
void BKE_brush_channelset_ensure_existing(BrushChannelSet *chset, BrushChannel *existing);

BrushChannel *BKE_brush_channelset_lookup(BrushChannelSet *chset, const char *idname);

bool BKE_brush_channelset_has(BrushChannelSet *chset, const char *idname);

BrushChannel *BKE_brush_channelset_add_builtin(BrushChannelSet *chset, const char *idname);
BrushChannel *BKE_brush_channelset_ensure_builtin(BrushChannelSet *chset, const char *idname);

void BKE_brush_channelset_merge(BrushChannelSet *dst,
                                BrushChannelSet *child,
                                BrushChannelSet *parent);

void BKE_brush_resolve_channels(struct Brush *brush, struct Sculpt *sd);

void BKE_brush_channelset_set_final_int(BrushChannelSet *brushset,
                                        BrushChannelSet *toolset,
                                        const char *idname,
                                        int value);

int BKE_brush_channelset_get_final_int(BrushChannelSet *brushset,
                                       BrushChannelSet *toolset,
                                       const char *idname,
                                       BrushMappingData *mapdata);

int BKE_brush_channelset_get_int(BrushChannelSet *chset,
                                 const char *idname,
                                 BrushMappingData *mapdata);
bool BKE_brush_channelset_set_int(BrushChannelSet *chset, const char *idname, int val);

void BKE_brush_channel_set_int(BrushChannel *ch, int val);
float BKE_brush_channel_get_int(BrushChannel *ch, BrushMappingData *mapdata);

// mapdata is optional, can be NULL

/* mapdata may be NULL */
float BKE_brush_channel_get_float(BrushChannel *ch, BrushMappingData *mapdata);
void BKE_brush_channel_set_float(BrushChannel *ch, float val);

/* mapdata may be NULL */
float BKE_brush_channelset_get_float(BrushChannelSet *chset,
                                     const char *idname,
                                     BrushMappingData *mapdata);
bool BKE_brush_channelset_set_float(BrushChannelSet *chset, const char *idname, float val);

float BKE_brush_channelset_get_final_float(BrushChannelSet *child,
                                           BrushChannelSet *parent,
                                           const char *idname,
                                           BrushMappingData *mapdata);

void BKE_brush_channelset_set_final_float(BrushChannelSet *child,
                                          BrushChannelSet *parent,
                                          const char *idname,
                                          float value);

void BKE_brush_channel_set_vector(BrushChannel *ch, float vec[4]);
int BKE_brush_channel_get_vector_size(BrushChannel *ch);

float BKE_brush_channel_curve_evaluate(BrushChannel *ch, float val, const float maxval);
CurveMapping *BKE_brush_channel_curvemapping_get(BrushCurve *curve, bool force_create);
bool BKE_brush_channel_curve_ensure_write(BrushCurve *curve);
void BKE_brush_channel_curve_assign(BrushChannel *ch, BrushCurve *curve);

/* returns size of vector */
int BKE_brush_channel_get_vector(BrushChannel *ch, float out[4], BrushMappingData *mapdata);

float BKE_brush_channelset_get_final_vector(BrushChannelSet *brushset,
                                            BrushChannelSet *toolset,
                                            const char *idname,
                                            float r_vec[4],
                                            BrushMappingData *mapdata);
void BKE_brush_channelset_set_final_vector(BrushChannelSet *brushset,
                                           BrushChannelSet *toolset,
                                           const char *idname,
                                           float vec[4]);
int BKE_brush_channelset_get_vector(BrushChannelSet *chset,
                                    const char *idname,
                                    float r_vec[4],
                                    BrushMappingData *mapdata);
bool BKE_brush_channelset_set_vector(BrushChannelSet *chset, const char *idname, float vec[4]);

void BKE_brush_init_toolsettings(struct Sculpt *sd);
void BKE_brush_builtin_create(struct Brush *brush, int tool);
BrushCommandList *BKE_brush_commandlist_create();
void BKE_brush_commandlist_free(BrushCommandList *cl);
BrushCommand *BKE_brush_commandlist_add(BrushCommandList *cl,
                                        BrushChannelSet *chset_template,
                                        bool auto_inherit);
BrushCommand *BKE_brush_command_init(BrushCommand *command, int tool);
void BKE_builtin_commandlist_create(struct Brush *brush,
                                    BrushChannelSet *chset,
                                    BrushCommandList *cl,
                                    int tool,
                                    BrushMappingData *map_data);  // map_data may be NULL
void BKE_brush_channelset_read(struct BlendDataReader *reader, BrushChannelSet *cset);
void BKE_brush_channelset_write(struct BlendWriter *writer, BrushChannelSet *cset);
void BKE_brush_channelset_read_lib(struct BlendLibReader *reader,
                                   struct ID *id,
                                   BrushChannelSet *chset);
void BKE_brush_channelset_expand(struct BlendExpander *expander,
                                 struct ID *id,
                                 BrushChannelSet *chset);
void BKE_brush_channelset_foreach_id(struct LibraryForeachIDData *data, BrushChannelSet *chset);

void BKE_brush_mapping_copy_data(BrushMapping *dst, BrushMapping *src);
const char *BKE_brush_mapping_type_to_str(BrushMappingType mapping);
const char *BKE_brush_mapping_type_to_typename(BrushMappingType mapping);

void BKE_brush_channelset_flag_clear(BrushChannelSet *chset, const char *channel, int flag);
void BKE_brush_channelset_flag_set(BrushChannelSet *chset, const char *channel, int flag);

/* adds missing channels to exising .channels in brush.
 * if channels do not exist use BKE_brush_builtin_create.
 */
void BKE_brush_builtin_patch(struct Brush *brush, int tool);

void BKE_brush_channelset_compat_load(BrushChannelSet *chset,
                                      struct Brush *brush,
                                      bool to_channels);

// merge in channels the ui requested
void BKE_brush_apply_queued_channels(BrushChannelSet *chset, bool do_override);
void BKE_brush_channeltype_rna_check(BrushChannelType *def,
                                     int (*getIconFromName)(const char *name));
bool BKE_brush_mapping_ensure_write(BrushMapping *mp);

void BKE_brush_channelset_apply_mapping(BrushChannelSet *chset, BrushMappingData *mapdata);
void BKE_brush_check_toolsettings(struct Sculpt *sd);
void BKE_brush_channelset_ui_init(struct Brush *brush, int tool);
void BKE_brush_channelset_check_radius(BrushChannelSet *chset);

/*
set up static type checker for BRUSHSET_XXX macros
*/
#define BRUSH_CHANNEL_DEFINE_EXTERNAL
#include "intern/brush_channel_define.h"
#undef BRUSH_CHANNEL_DEFINE_EXTERNAL

#ifdef __cplusplus
}
#endif
