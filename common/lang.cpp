//
// Created by ash on 21/11/24.
//

#include "lang.h"

config_strings get_config_strings(nn::swkbd::LanguageType language) {
    switch (language) {
        case nn::swkbd::LanguageType::English:
        default: return {
#include "en_US.lang"
            };
        case nn::swkbd::LanguageType::Spanish: return {
#include "es_ES.lang"
            };
        case nn::swkbd::LanguageType::French: return {
#include "fr_FR.lang"
            };
        case nn::swkbd::LanguageType::Italian: return {
#include "it_IT.lang"
            };
        case nn::swkbd::LanguageType::German: return {
#include "de_DE.lang"
            };
        case nn::swkbd::LanguageType::SimplifiedChinese: return {
#include "zh_CN.lang"
            };
        case nn::swkbd::LanguageType::TraditionalChinese: return {
#include "zh_Hant.lang"
            };
        case nn::swkbd::LanguageType::Portuguese: return {
#include "pt_BR.lang"
            };
        case nn::swkbd::LanguageType::Japanese: return {
#include "ja_JP.lang"
            };
        case nn::swkbd::LanguageType::Dutch: return {
#include "nl_NL.lang"
            };
        case nn::swkbd::LanguageType::Russian: return {
#include "ru_RU.lang"
            };
    }
}
