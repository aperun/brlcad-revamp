#include "common.h"

#include "bu.h"
#include "raytrace.h"

#include <cstdlib>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <set>
#include <map>

void
gen_attr_xml_list(const char *fname, const char *xref_id)
{
    int i = 0;
    std::ofstream fo;
    fo.open(fname);
    if (fo.bad()) {
	fo.close();
	bu_exit(1, "gen-attributes-files: File '%s' open error.\n", fname);
    }

    // the table files will be included in a parent DocBook xml file
    // for man pages and will be child elements of a DB <para>

    std::string title("Standard (Core) Attributes List");

    fo <<
        "<article xmlns='http://docbook.org/ns/docbook' version='5.0'\n"
        "  xmlns:xi='http://www.w3.org/2001/XInclude'\n"
        ">\n"
	"<!-- auto-generated by program 'gen-attributes-files' (misc/attributes/gen-attributes-files.cpp.in) -->\n"

        "  <info><title>" << title << "</title></info>\n"

        "  <para xml:id='" << xref_id << "'>\n"
        "    <variablelist remap='TP'>\n"
        ;

    // watch for an empty list
    while (!BU_STR_EQUAL(db5_attr_std[i].name, "")) {
        fo <<
            "       <varlistentry>\n"
            "	      <term><emphasis remap='B' role='bold'>" << db5_attr_std[i].name << ":</emphasis></term>\n"
            "	      <listitem>\n"
            "	        <para>" << db5_attr_std[i].long_description << "</para>\n"
            "	      </listitem>\n"
            "       </varlistentry>\n"
            ;
	i++;
    }

    fo <<
        "    </variablelist>\n"
        "  </para>\n"
        "</article>\n"
        ;

}

void
gen_attr_xml_table(const char *fname, const char *xref_id)
{
    int i = 0;
    std::ofstream fo;
    fo.open(fname);
    if (fo.bad()) {
	fo.close();
	bu_exit(1, "gen-attributes-files: File '%s' open error.\n", fname);
    }

    // the table files will be included in a parent DocBook xml file
    // for man pages and will be child elements of a DB <para>

    std::string title("Standard (Core) Attributes List");

    fo <<
        "<article xmlns='http://docbook.org/ns/docbook' version='5.0'\n"
        "  xmlns:xi='http://www.w3.org/2001/XInclude'\n"
        ">\n"
	"<!-- auto-generated by program 'gen-attributes-files' (misc/attributes/gen-attributes-files.cpp.in) -->\n"

        "  <info><title>" << title << "</title></info>\n"

        "  <para xml:id='" << xref_id << "'>\n"
        ;

    fo <<
        "  <table><title>" << title << "</title>\n"
        "    <tgroup cols='6'>\n"
        "      <colspec colname='c1'/>\n"
        "      <colspec colname='c2'/>\n"
        "      <colspec colname='c3'/>\n"
        "      <colspec colname='c4'/>\n"
        "      <colspec colname='c5'/>\n"
        "      <colspec colname='c6'/>\n"
        "      <thead>\n"
        "        <row>\n"
        "          <entry>Property</entry>\n"
        "          <entry>Attribute</entry>\n"
        "          <entry>Is Binary</entry>\n"
        "          <entry>Definition</entry>\n"
        "          <entry>Example</entry>\n"
        "          <entry>Aliases</entry>\n"
        "        </row>\n"
        "      </thead>\n"
        "      <tbody>\n"
	;

    while (!BU_STR_EQUAL(db5_attr_std[i].name, "")) {
        fo <<
            "        <row>\n"
            "          <entry>" << db5_attr_std[i].property	<< "</entry>\n"
            "          <entry>" << db5_attr_std[i].name		<< "</entry>\n"
            "          <entry>" << db5_attr_std[i].is_binary	<< "</entry>\n"
            "          <entry>" << db5_attr_std[i].description	<< "</entry>\n"
            "          <entry>" << db5_attr_std[i].examples	<< "</entry>\n"
            "          <entry>"
	    ;

	{
	    struct bu_vls alias;
	    const char *curr_pos, *next_pos;
	    bu_vls_init(&alias);
	    curr_pos = db5_attr_std[i].aliases;
	    while (curr_pos && strlen(curr_pos) > 0) {
		next_pos = strchr(curr_pos+1, ',');
		if (next_pos) {
		    bu_vls_strncpy(&alias, curr_pos, next_pos - curr_pos);
		    fo << bu_vls_addr(&alias) << ", ";
		    curr_pos = next_pos + 1;
		} else {
		    bu_vls_strncpy(&alias, curr_pos, strlen(curr_pos));
		    fo << bu_vls_addr(&alias);
		    curr_pos = NULL;
		}
	    }
	    bu_vls_free(&alias);
	}
	 fo <<
	    "</entry>\n"
            "        </row>\n"
            ;

	i++;
    }

    fo <<
        "      </tbody>\n"
        "    </tgroup>\n"
        "  </table>\n"
        "  </para>\n"
        "</article>\n"
        ;

    fo.close();
}

#if 0
void
gen_attr_html_page(const std::string& fname)
{
    std::ofstream fo;
    fo.open(fname.c_str());
    if (fo.bad()) {
	fo.close();
	bu_exit(1, "gen-attributes-files: File '%s' open error.\n", f.c_str());
    }

    fo <<
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <title>brlcad-attributes.html</title>\n"
        "  <meta charset = \"UTF-8\" />\n"
        "  <style type = \"text/css\">\n"
        "  table, td, th {\n"
        "    border: 1px solid black;\n"
        "  }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>BRL-CAD Standard and User-Registered Attributes</h1>\n"
        "  <p>Following are lists of the BRL-CAD standard and user-registered attribute names\n"
        "  along with their value definitions and aliases (if any).  Users should\n"
        "  not assign values to them in other than their defined format.\n"
        "  (Note that attribute names are not case-sensitive although their canonical form is\n"
        "  lower-case.)</p>\n"

        "  <p>Any code setting or reading the value of one of these attributes\n"
        "  must handle all aliases to ensure all functions asking for\n"
        "  the value in question get a consistent answer.</p>\n"

        "  <p>Some attributes have ASCII names but binary values (e.g., 'mtime').  Their values cannot\n"
        "  be modified by a user with the 'attr' command.  In some cases, but not all, their\n"
        "  values may be shown in a human readable form with the 'attr' command.)</p>\n"

        "  <p>If a user wishes to register an attribute to protect its use for models\n"
        "  transferred to other BRL-CAD users, submit the attribute, along with a description\n"
        "  of its intended use, to the\n"
        "  <a href=\"mailto:brlcad-devel@lists.sourceforge.net\">BRL-CAD developers</a>.\n"
        "  Its approval will be formal when it appears in the separate, registered-attribute\n"
        "  table following the standard attribute table.</p>\n"
        ;

    // need a table here (6 columns at the moment)
    fo <<
        "  <h3>Standard (Core) Attributes</h3>\n"
        "  <table>\n"
        "    <tr>\n"
        "      <th>Property</th>\n"
        "      <th>Attribute</th>\n"
        "      <th>Binary?</th>\n"
        "      <th>Definition</th>\n"
        "      <th>Example</th>\n"
        "      <th>Aliases</th>\n"
        "    </tr>\n"
        ;

    // track ATTR_REGISTERED type for separate listing
    map<int,db5_attr_t> rattrs;
    for (map<int,db5_attr_t>::iterator i = int2attr.begin();
         i != int2attr.end(); ++i) {
        db5_attr_t& a(i->second);
        if (a.attr_subtype == ATTR_REGISTERED) {
            rattrs.insert(make_pair(i->first,a));
            continue;
        }
        fo <<
            "    <tr>\n"
            "      <td>" << a.property                 << "</td>\n"
            "      <td>" << a.name                     << "</td>\n"
            "      <td>" << (a.is_binary ? "yes" : "") << "</td>\n"
            "      <td>" << a.description              << "</td>\n"
            "      <td>" << a.examples                 << "</td>\n"
            "      <td>"
            ;
        if (!a.aliases.empty()) {
            for (set<string>::iterator j = a.aliases.begin();
                 j != a.aliases.end(); ++j) {
                if (j != a.aliases.begin())
                    fo << ", ";
                fo << *j;
            }
        }
        fo <<
            "</td>\n"
            "    </tr>\n"
            ;
    }
    fo << "  </table>\n";

    // now show ATTR_REGISTERED types, if any
    fo << "  <h3>User-Registered Attributes</h3>\n";
    if (rattrs.empty()) {
        fo << "    <p>None at this time.</p>\n";
    } else {
        // need a table here
        fo <<
            "  <table>\n"
            "    <tr>\n"
            "      <th>Property</th>\n"
            "      <th>Attribute</th>\n"
            "      <th>Binary?</th>\n"
            "      <th>Definition</th>\n"
            "      <th>Example</th>\n"
            "      <th>Aliases</th>\n"
            "    </tr>\n"
            ;
        for (map<int,db5_attr_t>::iterator i = rattrs.begin(); i != rattrs.end(); ++i) {
            db5_attr_t& a(i->second);
            fo <<
                "    <tr>\n"
                "      <td>" << a.property                 << "</td>\n"
                "      <td>" << a.name                     << "</td>\n"
                "      <td>" << (a.is_binary ? "yes" : "") << "</td>\n"
                "      <td>" << a.description              << "</td>\n"
                "      <td>" << a.examples                 << "</td>\n"
                "      <td>"
                ;
            if (!a.aliases.empty()) {
                for (set<string>::iterator j = a.aliases.begin();
                     j != a.aliases.end(); ++i) {
                    if (j != a.aliases.begin())
                        fo << ", ";
                    fo << *j;
                }
            }
            fo <<
                "</td>\n"
                "    </tr>\n"
                ;
        }
        fo << "  </table>\n";
    }

    fo <<
        "</body>\n"
        "</html>\n"
        ;

    fo.close();

} // gen_attr_html_page
#endif

int
main(int argc, char **argv)
{
    int c = 0;
    int in_num = 0;
    int format = 0; /* 0 = XML, 1 = HTML */
    int list = 0;
    char outfile[MAXPATHLEN] = "attributes.xml";
    char xref_id[MAXPATHLEN] = "auto_attributes";
    const char *usage_str = "[-o output file] [-x xref id] [-f format number (0 = XML, 1 = HTML)] [-l]";

    while ((c=bu_getopt(argc, argv, "f:lo:x:")) != -1) {
	switch (c) {
	    case 'o' :
		memset(outfile, 0, MAXPATHLEN);
		bu_strlcpy(outfile, bu_optarg, MAXPATHLEN);
		break;
	    case 'x' :
		memset(xref_id, 0, MAXPATHLEN);
		bu_strlcpy(xref_id, bu_optarg, MAXPATHLEN);
		break;
	    case 'f' :
		sscanf(bu_optarg, "%d", &in_num);
		format = in_num;
		break;
	    case 'l' :
		list = 1;
		break;
	    default:
		bu_log("%s: %s\n", argv[0], usage_str);
		return -1;
	}
    }

    switch (format) {
	case 0:
	    if (list) {
		gen_attr_xml_list(outfile, xref_id);
	    } else {
		gen_attr_xml_table(outfile, xref_id);
	    }
	    break;
	case 1:
	    break;
	default:
	    bu_log("Error - unknown format %d\n", format);
	    return -1;
    }

    return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

