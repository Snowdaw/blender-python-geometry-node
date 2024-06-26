/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup edasset
 *
 * API to abstract away details for temporary loading of an ID from an asset. If the ID is stored
 * in the current file (or more precisely, in the #Main given when requesting an ID) no loading is
 * performed and the ID is returned. Otherwise it's imported for temporary access using the
 * `BLO_library_temp` API.
 */

#pragma once

#include "DNA_ID_enums.h"

struct AssetTempIDConsumer;
struct ID;
struct Main;
struct ReportList;
namespace blender::asset_system {
class AssetRepresentation;
}

namespace blender::ed::asset {

AssetTempIDConsumer *temp_id_consumer_create(
    const blender::asset_system::AssetRepresentation *asset);

void temp_id_consumer_free(AssetTempIDConsumer **consumer);
ID *temp_id_consumer_ensure_local_id(AssetTempIDConsumer *consumer,
                                     ID_Type id_type,
                                     Main *bmain,
                                     ReportList *reports);

}  // namespace blender::ed::asset
