// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLOCKEXPLORER_STYLE_H
#define BLOCKEXPLORER_STYLE_H

#include <string>

namespace BlockExplorer
{
class Style
{
public:
    // get <style>xxx</style> to inline into HTML
    static std::string getStyleInline();

    // get style css file content
    static std::string getStyleCssFileContent();

    // get filename to save css content
    static std::string getStyleCssFileName();

    // get <link>xxx</link> to inline into HTML to get style
    static std::string getStyleCssLink();

private:
    static std::string getStyleData();
};
}

#endif // BLOCKEXPLORER_STYLE_H
