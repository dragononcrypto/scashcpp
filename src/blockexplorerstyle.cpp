// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockexplorerstyle.h"

namespace BlockExplorer
{

std::string getStyleData()
{
    return (std::string) "body{background: #fff; font: 12px Lucida sans, Arial, Helvetica, sans-serif;color: #333;} " +
            " a{color: #2A679F;}.form-wrapper{background-color: #f6f6f6;background-image: -webkit-gradient(linear, left top, left bottom, from(#f6f6f6), to(#eae8e8));background-image: -webkit-linear-gradient(top, #f6f6f6, #eae8e8);background-image: -moz-linear-gradient(top, #f6f6f6, #eae8e8);background-image: -ms-linear-gradient(top, #f6f6f6, #eae8e8);background-image: -o-linear-gradient(top, #f6f6f6, #eae8e8);background-image: linear-gradient(top, #f6f6f6, #eae8e8);border-color: #dedede #bababa #aaa #bababa;border-style: solid;border-width: 1px;-webkit-border-radius: 10px;-moz-border-radius: 10px;border-radius: 10px;-webkit-box-shadow: 0 3px 3px rgba(255,255,255,.1), 0 3px 0 #bbb, 0 4px 0 #aaa, 0 5px 5px #444;-moz-box-shadow: 0 3px 3px rgba(255,255,255,.1), 0 3px 0 #bbb, 0 4px 0 #aaa, 0 5px 5px #444;box-shadow: 0 3px 3px rgba(255,255,255,.1), 0 3px 0 #bbb, 0 4px 0 #aaa, 0 5px 5px #444;margin:30px auto;overflow: hidden;padding: 8px;width: 450px;}"
            + " .form-wrapper #search{border: 1px solid #CCC;-webkit-box-shadow: 0 1px 1px #ddd inset, 0 1px 0 #FFF;-moz-box-shadow: 0 1px 1px #ddd inset, 0 1px 0 #FFF;box-shadow: 0 1px 1px #ddd inset, 0 1px 0 #FFF;-webkit-border-radius: 3px;-moz-border-radius: 3px;border-radius: 3px; color: #999;float: left;font: 16px Lucida Sans, Trebuchet MS, Tahoma, sans-serif;height: 40px;padding: 10px;width: 320px;}"
            + " .form-wrapper #search:focus{border-color: #aaa;-webkit-box-shadow: 0 1px 1px #bbb inset;-moz-box-shadow: 0 1px 1px #bbb inset;box-shadow: 0 1px 1px #bbb inset;outline: 0;}.form-wrapper #search:-moz-placeholder,.form-wrapper #search:-ms-input-placeholder,.form-wrapper #search::-webkit-input-placeholder{color: #999;font-weight: normal;}"
            + " .form-wrapper #submit{background-color: #0483a0;background-image: -webkit-gradient(linear, left top, left bottom, from(#31b2c3), to(#0483a0));background-image: -webkit-linear-gradient(top, #31b2c3, #0483a0);background-image: -moz-linear-gradient(top, #31b2c3, #0483a0);background-image: -ms-linear-gradient(top, #31b2c3, #0483a0);background-image: -o-linear-gradient(top, #31b2c3, #0483a0);background-image: linear-gradient(top, #31b2c3, #0483a0);border: 1px solid #00748f;-moz-border-radius: 3px;-webkit-border-radius: 3px;border-radius: 3px;-webkit-box-shadow: 0 1px 0 rgba(255, 255, 255, 0.3) inset, 0 1px 0 #FFF;-moz-box-shadow: 0 1px 0 rgba(255, 255, 255, 0.3) inset, 0 1px 0 #FFF;box-shadow: 0 1px 0 rgba(255, 255, 255, 0.3) inset, 0 1px 0 #FFF;color: #fafafa;cursor: pointer;height: 42px;float: right;font: 15px Arial, Helvetica;color: #999;padding: 0;text-transform: uppercase;text-shadow: 0 1px 0 rgba(0, 0 ,0, .3);width: 100px;}"
            + " .form-wrapper #submit:hover,.form-wrapper #submit:focus{background-color: #31b2c3;background-image: -webkit-gradient(linear, left top, left bottom, from(#0483a0), to(#31b2c3));background-image: -webkit-linear-gradient(top, #0483a0, #31b2c3);background-image: -moz-linear-gradient(top, #0483a0, #31b2c3);background-image: -ms-linear-gradient(top, #0483a0, #31b2c3);background-image: -o-linear-gradient(top, #0483a0, #31b2c3);background-image: linear-gradient(top, #0483a0, #31b2c3);}.form-wrapper #submit:active{-webkit-box-shadow: 0 1px 4px rgba(0, 0, 0, 0.5) inset;-moz-box-shadow: 0 1px 4px rgba(0, 0, 0, 0.5) inset;box-shadow: 0 1px 4px rgba(0, 0, 0, 0.5) inset;outline: 0;}.form-wrapper #submit::-moz-focus-inner{border: 0;} ul { list-style-type: none; padding-left:0; } label{ background-color: #AAAFAB; border-radius: 5px; padding: 3px; padding-left: 25px; color: white;	 }"
            + " li {  margin: 0px; padding: 5px; border-radius: 5px; } input[type=checkbox] { display: none; } input[type=checkbox] ~ ul { margin-top: 4px; max-height: 0; max-width: 0; opacity: 0; overflow: hidden; white-space:nowrap; -webkit-transition:all 200ms ease;  -moz-transition:all 200ms ease;  -o-transition:all 200ms ease;  transition:all 200ms ease; } "
            + " input[type=checkbox]:checked ~ ul {  max-height: 100%; max-width: 100%; opacity: 1; } input[type=checkbox] + label:before{ transform-origin:25% 50%; border: 7px solid transparent; border-width: 7px 12px;	 border-left-color: white; margin-left: -20px; width: 0; height: 0; display: inline-block; text-align: center; content: ''; color: #AAAFAB; -webkit-transition:all .5s ease;   -moz-transition:all .5s ease;   -o-transition:all .5s ease;   transition:all .5s ease;  position: absolute; margin-top: 1px; } "
            + " input[type=checkbox]:checked + label:before { transform: rotate(90deg); }";
}

std::string getStyleInline()
{
    return "<style>" + getStyleData() + "</style>";
}

std::string getStyleCssFileContent()
{
    return getStyleData();
}

std::string getStyleCssFileName()
{
    return "mystyle.css";
}

std::string getStyleCssLink()
{
    return "<link REL=\"StyleSheet\" TYPE=\"text/css\" HREF=\"" + getStyleCssFileName() + "\">";
}

}
