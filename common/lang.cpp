//
// Created by ash on 21/11/24.
//

#include "lang.h"

config_strings get_config_strings(inkay_language language) {
    switch (language) {
        case inkay_language::English:
        default: return {
#include "en_US.lang"
            };
        case inkay_language::Spanish: return {
#include "es_ES.lang"
            };
        case inkay_language::French: return {
#include "fr_FR.lang"
            };
        case inkay_language::Italian: return {
#include "it_IT.lang"
            };
        case inkay_language::German: return {
#include "de_DE.lang"
            };
        case inkay_language::SimplifiedChinese: return {
#include "zh_CN.lang"
            };
        case inkay_language::TraditionalChinese: return {
#include "zh_Hant.lang"
            };
        case inkay_language::Portuguese: return {
#include "pt_BR.lang"
            };
        case inkay_language::Japanese: return {
#include "ja_JP.lang"
            };
        case inkay_language::Dutch: return {
#include "nl_NL.lang"
            };
        case inkay_language::Russian: return {
#include "ru_RU.lang"
            };
    }
}
