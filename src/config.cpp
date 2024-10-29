/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2022 Ash Logan <ash@heyquark.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

bool Config::connect_to_network = false;
bool Config::initialized = false;
bool Config::shown_uninitialized_warning = false;

static config_strings strings;

config_strings get_config_strings(nn::swkbd::LanguageType language) {
    switch (language) {
        case nn::swkbd::LanguageType::English:
        default: return {
#include "lang/en_US.lang"
        };
        case nn::swkbd::LanguageType::Spanish: return {
#include "lang/es_ES.lang"
        };
        case nn::swkbd::LanguageType::French: return {
#include "lang/fr_FR.lang"
        };
        case nn::swkbd::LanguageType::Italian: return {
#include "lang/it_IT.lang"
        };
        case nn::swkbd::LanguageType::German: return {
#include "lang/de_DE.lang"
        };
        case nn::swkbd::LanguageType::SimplifiedChinese: return {
#include "lang/zh_CN.lang"
        };
        case nn::swkbd::LanguageType::TraditionalChinese: return {
#include "lang/zh_Hant.lang"
        };
        case nn::swkbd::LanguageType::Portuguese: return {
#include "lang/pt_BR.lang"
        };
        case nn::swkbd::LanguageType::Japanese: return {
#include "lang/ja_JP.lang"
        };
        case nn::swkbd::LanguageType::Dutch: return {
#include "lang/nl_NL.lang"
        };
        case nn::swkbd::LanguageType::Russian: return {
#include "lang/ru_RU.lang"
        };
    }
}
