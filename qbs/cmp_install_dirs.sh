#!/bin/sh
#############################################################################
##
## Copyright (C) 2016 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the build configuration tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 3 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL3 included in the
## packaging of this file. Please review the following information to
## ensure the GNU Lesser General Public License version 3 requirements
## will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 2.0 or (at your option) the GNU General
## Public license version 3 or any later version approved by the KDE Free
## Qt Foundation. The licenses are as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-2.0.html and
## https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

test $# -eq 2 || { echo "Usage: $0 <install dir 1> <install dir 2>"; exit 1; }
install_dir1="$1"
install_dir2="$2"

# Step 1: Compare list of files
files_list1=$(mktemp || exit 1)
files_list2=$(mktemp || exit 1)
cd "$install_dir1" || exit 1
find . |sort > "$files_list1" || exit 1
cd -
cd "$install_dir2" || exit 1
find . |sort > "$files_list2" || exit 1
cmp -s "$files_list1" "$files_list2" || { diff "$files_list1" "$files_list2" || exit 1; }

# Step 2: Compare contents of module headers
modules_list=$(ls $install_dir1/include || exit 1)
for module in $modules_list
do
    header1="$install_dir1/include/$module/$module"
    header2="$install_dir2/include/$module/$module"
    cmp -s "$header1 "$header2 || {
        echo "Unexpected difference in module header $module!"
        diff "$header1" "$header2"
        exit 1
    }
    header1="$install_dir1/include/$module/${module}Depends"
    header2="$install_dir2/include/$module/${module}Depends"
    cmp -s "$header1 "$header2 || {
        echo "Unexpected difference in module header $module!"
        diff "$header1" "$header2"
        exit 1
    }
done

# Step 3: Compare contents of module pri files
pri_file_list=$(ls $install_dir1/mkspecs/modules/*.pri || exit 1)
for pri_file in $pri_files_list
do
    pri_file1="$install_dir1/mkspecs/modules/$pri_file"
    pri_file1="$install_dir2/mkspecs/modules/$pri_file"
    cmp -s "$pri_file1 "$pri_file2 || {
        echo "Unexpected difference in module module pri file $pri_file!"
        diff "$pri_file" "$pri_file2"
        exit 1
    }
done
