/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VGM_IMPORT_OPTIONS_H
#define VGM_IMPORT_OPTIONS_H

extern struct VGM_ImporterOptions
{
    bool maximiseVolume = true;
    bool ignoreLfoFrequencyChanges = false;
    bool ignoreLfoAmplitudeChanges = false;
} g_vgmImportOptions;

#endif // VGM_IMPORT_OPTIONS_H
