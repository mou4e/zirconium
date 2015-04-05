// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/drive_metrics_provider.h"

#include <linux/kdev_t.h>  // For MAJOR()/MINOR().
#include <sys/stat.h>
#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"

#if defined(OS_CHROMEOS)
#include "base/sys_info.h"
#endif

namespace {

// See http://www.kernel.org/doc/Documentation/devices.txt for more info.
const int kFirstScsiMajorNumber = 8;
const int kPartitionsPerScsiDevice = 16;
const char kRotationalFormat[] = "/sys/block/sd%c/queue/rotational";

}  // namespace

// static
bool DriveMetricsProvider::HasSeekPenalty(const base::FilePath& path,
                                          bool* has_seek_penalty) {
#if defined(OS_CHROMEOS)
  std::string board = base::SysInfo::GetLsbReleaseBoard();
  if (board != "unknown" && board != "parrot") {
    // All ChromeOS devices have SSDs. Except some parrots.
    *has_seek_penalty = false;
    return true;
  }
#endif

  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return false;

  struct stat path_stat;
  int error = fstat(file.GetPlatformFile(), &path_stat);
  if (error < 0 || MAJOR(path_stat.st_dev) != kFirstScsiMajorNumber) {
    // TODO(dbeam): support more SCSI major numbers (e.g. /dev/sdq+) and LVM?
    return false;
  }

  char sdX = 'a' + MINOR(path_stat.st_dev) / kPartitionsPerScsiDevice;
  std::string rotational_path = base::StringPrintf(kRotationalFormat, sdX);
  std::string rotates;
  if (!base::ReadFileToString(base::FilePath(rotational_path), &rotates))
    return false;

  *has_seek_penalty = rotates.substr(0, 1) == "1";
  return true;
}