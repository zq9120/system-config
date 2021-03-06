/** @file scim_fcitx_imengine.cpp
 * implementation of class FcitxInstance.
 */

/*
 * Smart Common Input Method 
 * and Fcitx ported on to it 
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn> /author of scim
 * Yuking <yuking_net@sohu.com> /author of fcitx
 * Bao Haojun <baohaojun@yahoo.com> /ported fcitx onto scim
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * $Id: scim_fcitx_imengine.cpp,v 1.6 2005/05/20 14:41:12 baohaojun Exp $
 *
 */

#define Uses_SCIM_IMENGINE
#define Uses_SCIM_ICONV
#define Uses_SCIM_CONFIG_BASE
#define Uses_SCIM_CONFIG_PATH

#include <scim.h>
#include "scim_fcitx_imengine.h"
#include <string.h>
#include <ctype.h>
#include "ime-socket.h"
#define ENABLE_BHJDEBUG
#include "bhjdebug.h" 
#include <vector>
using std::vector;

#define scim_module_init fcitx_LTX_scim_module_init
#define scim_module_exit fcitx_LTX_scim_module_exit
#define scim_imengine_module_init fcitx_LTX_scim_imengine_module_init
#define scim_imengine_module_create_factory fcitx_LTX_scim_imengine_module_create_factory

#define SCIM_CONFIG_IMENGINE_FCITX_LANGUAGES "/IMEngine/Fcitx/Languages"

#define SCIM_PROP_STATUS                       		"/IMEngine/Fcitx/Status"
#define SCIM_PROP_LETTER                                "/IMEngine/Fcitx/Letter"
#define SCIM_PROP_PUNCT                                 "/IMEngine/Fcitx/Punct"
#define SCIM_PROP_GBK 					"/IMEngine/Fcitx/Gbk"
#define SCIM_PROP_LEGEND				"/IMEngine/Fcitx/Legend"
#define SCIM_PROP_LOCK					"/IMEngine/Fcitx/Lock"

#define SCIM_FULL_LETTER_ICON                             (SCIM_ICONDIR "/full-letter.png")
#define SCIM_HALF_LETTER_ICON                             (SCIM_ICONDIR "/half-letter.png")
#define SCIM_FULL_PUNCT_ICON                              (SCIM_ICONDIR "/full-punct.png")
#define SCIM_HALF_PUNCT_ICON                              (SCIM_ICONDIR "/half-punct.png")
#define SCIM_STATUS_ICON				  (SCIM_ICONDIR "/%s%s.png")
#define SCIM_GBK_ICON				  	  (SCIM_ICONDIR "/%sgbk.png")
#define SCIM_LEGEND_ICON				  (SCIM_ICONDIR "/%slegend.png")
#define SCIM_LOCK_ICON					  (SCIM_ICONDIR "/%slock.png")

#define SCIM_FCITX_ICON_FILE                 (SCIM_ICONDIR "/fcitx.png")



using namespace scim;

static Pointer <FcitxFactory> _scim_fcitx_factory;
static ConfigPointer _scim_config;

static const char * _DEFAULT_LANGUAGES = (
    "zh_CN,zh_TW,zh_HK,zh_SG");

extern "C" {
    void scim_module_init (void)
    {
    }

    void scim_module_exit (void)
    {
        _scim_fcitx_factory.reset ();
        _scim_config.reset ();
    }

    unsigned int scim_imengine_module_init (const ConfigPointer &config)
    {
        _scim_config = config;
        return 1;
    }

    IMEngineFactoryPointer scim_imengine_module_create_factory (unsigned int factory)
    {
        String languages;

        if (factory != 0) return NULL;

        if (!_scim_config.null ()) {
            languages = _scim_config->read (
				String (SCIM_CONFIG_IMENGINE_FCITX_LANGUAGES),
				String ("default"));
        } else {
            languages = String ("default");
        }

        if (_scim_fcitx_factory.null ()) {
            _scim_fcitx_factory =
                new FcitxFactory (utf8_mbstowcs (String (("FCIM"))), languages);
        }
        return _scim_fcitx_factory;
    }
}

#ifdef SF_DEBUG
void FCIM_DEUBG()
{

}
#endif

// implementation of Fcitx
FcitxFactory::FcitxFactory ()
{
    m_name = utf8_mbstowcs (("FCIM"));
    set_languages (String ((_DEFAULT_LANGUAGES)));
}

IConvert FcitxInstance::m_gbiconv("GB18030");
FcitxFactory::FcitxFactory (const WideString& name,
							const String& languages)
{
    // do not allow too long name
    if (name.length () <= 8)
        m_name = name;
    else
        m_name.assign (name.begin (), name.begin () + 8);

    if (languages == String ("default"))
        set_languages (String ((_DEFAULT_LANGUAGES)));
    else
        set_languages (languages);
}

FcitxFactory::~FcitxFactory ()
{
}

WideString
FcitxFactory::get_name () const
{
    return m_name;
}

WideString
FcitxFactory::get_authors () const
{
    return utf8_mbstowcs (String (
							  ("(C) 2002-2004 James Su <suzhe@tsinghua.org.cn>")));
}

WideString
FcitxFactory::get_credits () const
{
    return WideString ();
}

WideString
FcitxFactory::get_help () const
{
    return utf8_mbstowcs (String ((
									  "Hot Keys:\n\n"
									  "  Control+u:\n"
									  "    switch between Multibyte encoding and Unicode.\n\n"
									  "  Control+comma:\n"
									  "    switch between full/half width punctuation mode.\n\n"
									  "  Shift+space:\n"
									  "    switch between full/half width letter mode.\n\n"
									  "  Esc:\n"
									  "    reset the input method.\n")));
}

String
FcitxFactory::get_uuid () const
{
    return String ("39f707ce-b3e0-4e3a-8dd8-a1afb886a9c9");
}

String
FcitxFactory::get_icon_file () const
{
    return String (SCIM_FCITX_ICON_FILE);
}

String
FcitxFactory::get_language () const
{
    return scim_validate_language ("other");
}

IMEngineInstancePointer
FcitxFactory::create_instance (const String& encoding, int id)
{
    return new FcitxInstance (this, encoding, id);
}

int
FcitxFactory::get_maxlen (const String &encoding)
{
    std::vector <String> locales;

    scim_split_string_list (locales, get_locales ());

    for (unsigned int i=0; i<locales.size (); ++i)
        if (scim_get_locale_encoding (locales [i]) == encoding)
            return scim_get_locale_maxlen (locales [i]);
    return 1;
}

// implementation of FcitxInstance
FcitxInstance::FcitxInstance (FcitxFactory *factory,
							  const String& encoding,
							  int id)
    : IMEngineInstanceBase (factory, encoding, id),
	  m_forward (false),
	  m_focused (false),
	  m_iconv (encoding)
{
}

FcitxInstance::~FcitxInstance ()
{

}



static vector<string> str_split_space(const string& str)
{
	size_t start = 0;
	vector<string> res;
	if (str.empty()) {
		return res;
	}
	for (;;) {
		size_t spc = str.find(' ', start);
		if (spc == string::npos) {
			res.push_back(str.substr(start));
			break;
		}
		res.push_back(str.substr(start, spc-start));
		start = spc+1;
	}
	return res;
}

static string str_percent_decode(const string& str)
{
	string res;
	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] != '%') {
			res.push_back(str[i]);
		} else if (i + 2 < str.size()) {
			string hex = str.substr(i+1, 2);
			char c = (char) strtol(hex.c_str(), NULL, 16);
			res.push_back(c);
			i += 2; //there's still a i++ in the for statement
		} else {
			break;
		}			
	}
	return res;
}

void FcitxInstance::DisplayInputWindow(const ime_client& client)
{

	compstr = client.compstr;

	if (!client.hintstr.empty()) {
		update_aux_string(utf8_mbstowcs(client.hintstr), AttributeList());
		show_aux_string();
    } else {
		hide_aux_string();
    }

	if (!client.commitstr.empty()) {
		send_string(client.commitstr);
	}
    if (client.candsstr.empty()) {
		if (!client.compstr.empty()) {
			update_preedit_string(utf8_mbstowcs(client.compstr), AttributeList());
			show_preedit_string();
		} else {
			hide_preedit_string();
		}

		hide_lookup_table();
		return;
    }

	vector<string> cands = str_split_space(client.candsstr);

	size_t idx = atoi(client.cand_idx.c_str());
	idx %= 10;


    m_lookup_table.clear();
    for (size_t i=0; i < cands.size(); i++) {

		AttributeList attributes;
		WideString cand_wstr = utf8_mbstowcs(str_percent_decode(cands[i]));

		if (i == idx) {
			attributes.push_back(Attribute(0, cand_wstr.size(), SCIM_ATTR_BACKGROUND, SCIM_RGB_COLOR(0, 200, 50)));
			update_preedit_string(cand_wstr, AttributeList());
			show_preedit_string();
		}
		m_lookup_table.append_candidate(cand_wstr, attributes);
    }

    m_lookup_table.set_page_size(10);
	m_lookup_table.fix_page_size(10);
    update_lookup_table(m_lookup_table);
    show_lookup_table();
		
}

void FcitxInstance::send_string(const string& str)
{
    commit_string(utf8_mbstowcs(str));
}


static String get_complex_key_name(const KeyEvent& key)
{
	String name;
	KeyEvent stripped_key(key.code);

	if (key.is_control_down()) {
		name += "C ";
	} 

	if (key.is_shift_down() && !isgraph(stripped_key.get_ascii_code())) {
		name += "S ";
	}

	if (key.is_alt_down()) {
		name += "A ";
	}


	if (isgraph(stripped_key.get_ascii_code())) {
		name.push_back(stripped_key.get_ascii_code());
	} else {
		String stripped_str = stripped_key.get_key_string();

		for (int i = 0; i < stripped_str.size(); i++) {
			name.push_back(tolower(stripped_str[i]));
		}
	}
	return name;
}

bool string_begin_with(const string& src, const string& dst)
{
	if (src.size() < dst.size()) {
		return false;
	}

	if (!strncmp(src.c_str(), dst.c_str(), dst.size())) {
		return true;
	}
	return false;
}

ime_client get_client_reply()
{
	ime_client client;
	for (;;) {
		string str = ime_recv_line();
		if (str.empty() || str == "end:") {
			break;
		}

		if (string_begin_with(str, "commit: ")) {
			client.commitstr = str.substr(strlen("commit: "));
		} else if (string_begin_with(str, "hint: ")) {
			client.hintstr = str.substr(strlen("hint: "));
		} else if (string_begin_with(str, "comp: ")) {
			client.compstr = str.substr(strlen("comp: "));
		} else if (string_begin_with(str, "cands: ")) {
			client.candsstr = str.substr(strlen("cands: "));
		} else if (string_begin_with(str, "cand_index: ")) {
			client.cand_idx = str.substr(strlen("cand_index: "));
		} else if (string_begin_with(str, "active: ")) {
			client.activestr = str.substr(strlen("active: "));
		} else if (string_begin_with(str, "beep: ")) {
			client.beepstr = str.substr(strlen("beep: "));
		} else {
			client.hintstr = str;
			//beep();
		}
	}
	return client;
}


void 
FcitxInstance::translate_key(const string& key_desc)
{

	ime_write_line(string("keyed ") + key_desc);
	ime_client client = get_client_reply();

	DisplayInputWindow(client);
}

bool
FcitxInstance::process_key_event (const KeyEvent& key)
{

	static bool ime_server_inited = false;
	if (!ime_server_inited) {
		connect_ime_server();
		ime_server_inited = true;
	}
    if (!m_focused) return false;

    if (key.is_key_release()) {
		return false;
	}

	if (compstr.empty()) {
		if (key.is_control_down()
			|| key.is_alt_down()
			|| key.is_meta_down()
			|| key.is_super_down()
			|| key.is_hyper_down()) {
			return false;
		} else if (!isgraph(key.get_ascii_code())) {
			return false;
		}
	}
		
	String key_name;
	int ascii_code = key.get_ascii_code();

	key_name = get_complex_key_name(key);

	if (key_name.empty()) {
		return false;
	}
	translate_key(key_name);
	return true;
}

void
FcitxInstance::select_candidate (unsigned int item)
{
    WideString label = m_lookup_table.get_candidate_label (item);
    KeyEvent key ((int) label [0], 0);
    process_key_event (key);
}

void
FcitxInstance::update_lookup_table_page_size (unsigned int page_size)
{
    if (page_size > 0)
        m_lookup_table.set_page_size (page_size);
}

void
FcitxInstance::lookup_table_page_up ()
{

}

void
FcitxInstance::lookup_table_page_down ()
{

}

void
FcitxInstance::move_preedit_caret (unsigned int /*pos*/)
{
}

void
FcitxInstance::reset ()
{
    m_iconv.set_encoding (get_encoding ());
    m_lookup_table.clear ();

    hide_lookup_table ();
    hide_preedit_string ();
}

void
FcitxInstance::focus_in ()
{
    m_focused = true;
	//ime_write_line("keyed C \\");
    //fixme DisplayInputWindow();
}

void
FcitxInstance::focus_out ()
{
	//ime_write_line("keyed C \\");
    m_focused = false;
}

void
FcitxInstance::trigger_property (const String &property)
{
}

/*
  vi:ts=4:nowrap:ai:expandtab
*/
