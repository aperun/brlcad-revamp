<?xml version="1.0" encoding="ASCII"?>
<!--This file was created automatically by html2xhtml-->
<!--from the HTML stylesheets.-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:d="http://docbook.org/ns/docbook"
xmlns:suwl="http://nwalsh.com/xslt/ext/com.nwalsh.saxon.UnwrapLinks" xmlns:exsl="http://exslt.org/common" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns="http://www.w3.org/1999/xhtml" exclude-result-prefixes="suwl exsl xlink d" version="1.0">

<!-- ********************************************************************
     $Id: xref.xsl 8421 2009-05-04 07:49:49Z bobstayton $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- Use internal variable for olink xlink role for consistency -->
<xsl:variable name="xolink.role">http://docbook.org/xlink/role/olink</xsl:variable>

<!-- ==================================================================== -->

<xsl:template match="d:anchor">
  <xsl:call-template name="anchor"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="d:xref" name="xref">
  <xsl:param name="xhref" select="@xlink:href"/>
  <!-- is the @xlink:href a local idref link? -->
  <xsl:param name="xlink.idref">
    <xsl:if test="starts-with($xhref,'#')                   and (not(contains($xhref,'('))                   or starts-with($xhref, '#xpointer(id('))">
      <xsl:call-template name="xpointer.idref">
        <xsl:with-param name="xpointer" select="$xhref"/>
      </xsl:call-template>
   </xsl:if>
  </xsl:param>
  <xsl:param name="xlink.targets" select="key('id',$xlink.idref)"/>
  <xsl:param name="linkend.targets" select="key('id',@linkend)"/>
  <xsl:param name="target" select="($xlink.targets | $linkend.targets)[1]"/>

  <xsl:variable name="xrefstyle">
    <xsl:choose>
      <xsl:when test="@role and not(@xrefstyle)                        and $use.role.as.xrefstyle != 0">
        <xsl:value-of select="@role"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="@xrefstyle"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:call-template name="anchor"/>

  <xsl:variable name="content">
    <xsl:choose>
  
      <xsl:when test="@endterm">
        <xsl:variable name="etargets" select="key('id',@endterm)"/>
        <xsl:variable name="etarget" select="$etargets[1]"/>
        <xsl:choose>
          <xsl:when test="count($etarget) = 0">
            <xsl:message>
              <xsl:value-of select="count($etargets)"/>
              <xsl:text>Endterm points to nonexistent ID: </xsl:text>
              <xsl:value-of select="@endterm"/>
            </xsl:message>
            <xsl:text>???</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="$etarget" mode="endterm"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
  
      <xsl:when test="$target/@xreflabel">
        <xsl:call-template name="xref.xreflabel">
          <xsl:with-param name="target" select="$target"/>
        </xsl:call-template>
      </xsl:when>
  
      <xsl:when test="$target">
        <xsl:if test="not(parent::d:citation)">
          <xsl:apply-templates select="$target" mode="xref-to-prefix"/>
        </xsl:if>
  
        <xsl:apply-templates select="$target" mode="xref-to">
          <xsl:with-param name="referrer" select="."/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        </xsl:apply-templates>
  
        <xsl:if test="not(parent::d:citation)">
          <xsl:apply-templates select="$target" mode="xref-to-suffix"/>
        </xsl:if>
      </xsl:when>

      <xsl:otherwise>
        <xsl:message>
          <xsl:text>ERROR: xref linking to </xsl:text>
          <xsl:value-of select="@linkend|@xlink:href"/>
          <xsl:text> has no generated link text.</xsl:text>
        </xsl:message>
        <xsl:text>???</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:call-template name="simple.xlink">
    <xsl:with-param name="content" select="$content"/>
  </xsl:call-template>

</xsl:template>

<!-- ==================================================================== -->

<!-- biblioref handled largely like an xref -->
<!-- To be done: add support for begin, end, and units attributes -->
<xsl:template match="d:biblioref">
  <xsl:variable name="targets" select="key('id',@linkend)"/>
  <xsl:variable name="target" select="$targets[1]"/>
  <xsl:variable name="refelem" select="local-name($target)"/>

  <xsl:call-template name="check.id.unique">
    <xsl:with-param name="linkend" select="@linkend"/>
  </xsl:call-template>

  <xsl:call-template name="anchor"/>

  <xsl:choose>
    <xsl:when test="count($target) = 0">
      <xsl:message>
        <xsl:text>XRef to nonexistent id: </xsl:text>
        <xsl:value-of select="@linkend"/>
      </xsl:message>
      <xsl:text>???</xsl:text>
    </xsl:when>

    <xsl:when test="@endterm">
      <xsl:variable name="href">
        <xsl:call-template name="href.target">
          <xsl:with-param name="object" select="$target"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="etargets" select="key('id',@endterm)"/>
      <xsl:variable name="etarget" select="$etargets[1]"/>
      <xsl:choose>
        <xsl:when test="count($etarget) = 0">
          <xsl:message>
            <xsl:value-of select="count($etargets)"/>
            <xsl:text>Endterm points to nonexistent ID: </xsl:text>
            <xsl:value-of select="@endterm"/>
          </xsl:message>
          <a href="{$href}">
            <xsl:apply-templates select="." mode="common.html.attributes"/>
            <xsl:text>???</xsl:text>
          </a>
        </xsl:when>
        <xsl:otherwise>
          <a href="{$href}">
            <xsl:apply-templates select="." mode="common.html.attributes"/>
            <xsl:apply-templates select="$etarget" mode="endterm"/>
          </a>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="$target/@xreflabel">
      <a>
        <xsl:apply-templates select="." mode="common.html.attributes"/>
        <xsl:attribute name="href">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$target"/>
          </xsl:call-template>
        </xsl:attribute>
        <xsl:call-template name="xref.xreflabel">
          <xsl:with-param name="target" select="$target"/>
        </xsl:call-template>
      </a>
    </xsl:when>

    <xsl:otherwise>
      <xsl:variable name="href">
        <xsl:call-template name="href.target">
          <xsl:with-param name="object" select="$target"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:if test="not(parent::d:citation)">
        <xsl:apply-templates select="$target" mode="xref-to-prefix"/>
      </xsl:if>

      <a href="{$href}">
        <xsl:apply-templates select="." mode="class.attribute"/>
        <xsl:if test="$target/d:title or $target/*/d:title">
          <xsl:attribute name="title">
            <xsl:apply-templates select="$target" mode="xref-title"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:apply-templates select="$target" mode="xref-to">
          <xsl:with-param name="referrer" select="."/>
          <xsl:with-param name="xrefstyle">
            <xsl:choose>
              <xsl:when test="@role and not(@xrefstyle) and $use.role.as.xrefstyle != 0">
                <xsl:value-of select="@role"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="@xrefstyle"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:with-param>
        </xsl:apply-templates>
      </a>

      <xsl:if test="not(parent::d:citation)">
        <xsl:apply-templates select="$target" mode="xref-to-suffix"/>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="endterm">
  <!-- Process the children of the endterm element -->
  <xsl:variable name="endterm">
    <xsl:apply-templates select="child::node()"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$exsl.node.set.available != 0">
      <xsl:apply-templates select="exsl:node-set($endterm)" mode="remove-ids"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$endterm"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="*" mode="remove-ids">
  <xsl:choose>
    <!-- handle html or xhtml -->
    <xsl:when test="local-name(.) = 'a'                     and (namespace-uri(.) = ''                          or namespace-uri(.) = 'http://www.w3.org/1999/xhtml')">
      <xsl:choose>
        <xsl:when test="(@name and count(@*) = 1)                         or (@id and count(@*) = 1)                         or (@xml:id and count(@*) = 1)                         or (@xml:id and @name and count(@*) = 2)                         or (@id and @name and count(@*) = 2)">
          <xsl:message>suppress anchor</xsl:message>
          <!-- suppress the whole thing -->
        </xsl:when>
        <xsl:otherwise>
          <xsl:copy>
            <xsl:for-each select="@*">
              <xsl:choose>
                <xsl:when test="local-name(.) != 'name' and local-name(.) != 'id'">
                  <xsl:copy/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:message>removing <xsl:value-of select="local-name(.)"/></xsl:message>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
          </xsl:copy>
          <xsl:apply-templates mode="remove-ids"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy>
        <xsl:for-each select="@*">
          <xsl:choose>
            <xsl:when test="local-name(.) != 'id'">
              <xsl:copy/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:message>removing <xsl:value-of select="local-name(.)"/></xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
        <xsl:apply-templates mode="remove-ids"/>
      </xsl:copy>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="xref-to-prefix"/>
<xsl:template match="*" mode="xref-to-suffix"/>

<xsl:template match="*" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:if test="$verbose">
    <xsl:message>
      <xsl:text>Don't know what gentext to create for xref to: "</xsl:text>
      <xsl:value-of select="name(.)"/>
      <xsl:text>", ("</xsl:text>
      <xsl:value-of select="(@id|@xml:id)[1]"/>
      <xsl:text>")</xsl:text>
    </xsl:message>
  </xsl:if>
  <xsl:text>???</xsl:text>
</xsl:template>

<xsl:template match="d:title" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <!-- if you xref to a title, xref to the parent... -->
  <xsl:choose>
    <!-- FIXME: how reliable is this? -->
    <xsl:when test="contains(local-name(parent::*), 'info')">
      <xsl:apply-templates select="parent::*[2]" mode="xref-to">
        <xsl:with-param name="referrer" select="$referrer"/>
        <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        <xsl:with-param name="verbose" select="$verbose"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="parent::*" mode="xref-to">
        <xsl:with-param name="referrer" select="$referrer"/>
        <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        <xsl:with-param name="verbose" select="$verbose"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:abstract|d:authorblurb|d:personblurb|d:bibliodiv|d:bibliomset                      |d:biblioset|d:blockquote|d:calloutlist|d:caution|d:colophon                      |d:constraintdef|d:formalpara|d:glossdiv|d:important|d:indexdiv                      |d:itemizedlist|d:legalnotice|d:lot|d:msg|d:msgexplan|d:msgmain                      |d:msgrel|d:msgset|d:msgsub|d:note|d:orderedlist|d:partintro                      |d:productionset|d:qandadiv|d:refsynopsisdiv|d:segmentedlist                      |d:set|d:setindex|d:sidebar|d:tip|d:toc|d:variablelist|d:warning" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <!-- catch-all for things with (possibly optional) titles -->
  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:author|d:editor|d:othercredit|d:personname" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <xsl:call-template name="person.name"/>
</xsl:template>

<xsl:template match="d:authorgroup" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <xsl:call-template name="person.name.list"/>
</xsl:template>

<xsl:template match="d:figure|d:example|d:table|d:equation" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:procedure" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:task" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:cmdsynopsis" mode="xref-to">
  <xsl:apply-templates select="(.//d:command)[1]" mode="xref"/>
</xsl:template>

<xsl:template match="d:funcsynopsis" mode="xref-to">
  <xsl:apply-templates select="(.//d:function)[1]" mode="xref"/>
</xsl:template>

<xsl:template match="d:dedication|d:acknowledgements|d:preface|d:chapter|d:appendix|d:article" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:bibliography" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:biblioentry|d:bibliomixed" mode="xref-to-prefix">
  <xsl:text>[</xsl:text>
</xsl:template>

<xsl:template match="d:biblioentry|d:bibliomixed" mode="xref-to-suffix">
  <xsl:text>]</xsl:text>
</xsl:template>

<xsl:template match="d:biblioentry|d:bibliomixed" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <!-- handles both biblioentry and bibliomixed -->
  <xsl:choose>
    <xsl:when test="string(.) = ''">
      <xsl:variable name="bib" select="document($bibliography.collection,.)"/>
      <xsl:variable name="id" select="(@id|@xml:id)[1]"/>
      <xsl:variable name="entry" select="$bib/d:bibliography/                                     *[@id=$id or @xml:id=$id][1]"/>
      <xsl:choose>
        <xsl:when test="$entry">
          <xsl:choose>
            <xsl:when test="$bibliography.numbered != 0">
              <xsl:number from="d:bibliography" count="d:biblioentry|d:bibliomixed" level="any" format="1"/>
            </xsl:when>
            <xsl:when test="local-name($entry/*[1]) = 'abbrev'">
              <xsl:apply-templates select="$entry/*[1]"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="(@id|@xml:id)[1]"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message>
            <xsl:text>No bibliography entry: </xsl:text>
            <xsl:value-of select="$id"/>
            <xsl:text> found in </xsl:text>
            <xsl:value-of select="$bibliography.collection"/>
          </xsl:message>
          <xsl:value-of select="(@id|@xml:id)[1]"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$bibliography.numbered != 0">
          <xsl:number from="d:bibliography" count="d:biblioentry|d:bibliomixed" level="any" format="1"/>
        </xsl:when>
        <xsl:when test="local-name(*[1]) = 'abbrev'">
          <xsl:apply-templates select="*[1]"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="(@id|@xml:id)[1]"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:glossary" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:glossentry" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>
  <xsl:choose>
    <xsl:when test="$glossentry.show.acronym = 'primary'">
      <xsl:choose>
        <xsl:when test="d:acronym|d:abbrev">
          <xsl:apply-templates select="(d:acronym|d:abbrev)[1]"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="d:glossterm[1]" mode="xref-to">
            <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
            <xsl:with-param name="referrer" select="$referrer"/>
            <xsl:with-param name="verbose" select="$verbose"/>
          </xsl:apply-templates>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="d:glossterm[1]" mode="xref-to">
        <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        <xsl:with-param name="referrer" select="$referrer"/>
        <xsl:with-param name="verbose" select="$verbose"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:glossterm|d:firstterm" mode="xref-to">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="d:index" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:listitem" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:section|d:simplesect                      |d:sect1|d:sect2|d:sect3|d:sect4|d:sect5                      |d:refsect1|d:refsect2|d:refsect3|d:refsection" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
  <!-- FIXME: What about "in Chapter X"? -->
</xsl:template>

<xsl:template match="d:bridgehead" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
  <!-- FIXME: What about "in Chapter X"? -->
</xsl:template>

<xsl:template match="d:qandaset" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:qandadiv" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:qandaentry" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="d:question[1]" mode="xref-to">
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:question|d:answer" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:choose>
    <xsl:when test="string-length(d:label) != 0">
      <xsl:apply-templates select="." mode="label.markup"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="." mode="object.xref.markup">
        <xsl:with-param name="purpose" select="'xref'"/>
        <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        <xsl:with-param name="referrer" select="$referrer"/>
        <xsl:with-param name="verbose" select="$verbose"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:part|d:reference" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:refentry" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <xsl:choose>
    <xsl:when test="d:refmeta/d:refentrytitle">
      <xsl:apply-templates select="d:refmeta/d:refentrytitle"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="d:refnamediv/d:refname[1]"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:apply-templates select="d:refmeta/d:manvolnum"/>
</xsl:template>

<xsl:template match="d:refnamediv" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="d:refname[1]" mode="xref-to">
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:refname" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates mode="xref-to"/>
</xsl:template>

<xsl:template match="d:step" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <xsl:call-template name="gentext">
    <xsl:with-param name="key" select="'Step'"/>
  </xsl:call-template>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="." mode="number"/>
</xsl:template>

<xsl:template match="d:varlistentry" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="d:term[1]" mode="xref-to">
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="d:primary|d:secondary|d:tertiary" mode="xref-to">
  <xsl:value-of select="."/>
</xsl:template>

<xsl:template match="d:indexterm" mode="xref-to">
  <xsl:value-of select="d:primary"/>
</xsl:template>

<xsl:template match="d:varlistentry/d:term" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <!-- to avoid the comma that will be generated if there are several terms -->
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="d:co" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <xsl:apply-templates select="." mode="callout-bug"/>
</xsl:template>

<xsl:template match="d:area|d:areaset" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>

  <xsl:call-template name="callout-bug">
    <xsl:with-param name="conum">
      <xsl:apply-templates select="." mode="conumber"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="d:book" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:apply-templates select="." mode="object.xref.markup">
    <xsl:with-param name="purpose" select="'xref'"/>
    <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
    <xsl:with-param name="referrer" select="$referrer"/>
    <xsl:with-param name="verbose" select="$verbose"/>
  </xsl:apply-templates>
</xsl:template>

<!-- These are elements for which no link text exists, so an xref to one
     uses the xrefstyle attribute if specified, or if not it falls back
     to the container element's link text -->
<xsl:template match="d:para|d:phrase|d:simpara|d:anchor|d:quote" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>

  <xsl:variable name="context" select="(ancestor::d:simplesect                                        |ancestor::d:section                                        |ancestor::d:sect1                                        |ancestor::d:sect2                                        |ancestor::d:sect3                                        |ancestor::d:sect4                                        |ancestor::d:sect5                                        |ancestor::d:refsection                                        |ancestor::d:refsect1                                        |ancestor::d:refsect2                                        |ancestor::d:refsect3                                        |ancestor::d:chapter                                        |ancestor::d:appendix                                        |ancestor::d:preface                                        |ancestor::d:partintro                                        |ancestor::d:dedication                                        |ancestor::d:acknowledgements                                        |ancestor::d:colophon                                        |ancestor::d:bibliography                                        |ancestor::d:index                                        |ancestor::d:glossary                                        |ancestor::d:glossentry                                        |ancestor::d:listitem                                        |ancestor::d:varlistentry)[last()]"/>

  <xsl:choose>
    <xsl:when test="$xrefstyle != ''">
      <xsl:apply-templates select="." mode="object.xref.markup">
        <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        <xsl:with-param name="referrer" select="$referrer"/>
        <xsl:with-param name="verbose" select="$verbose"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="$context" mode="xref-to">
        <xsl:with-param name="purpose" select="'xref'"/>
        <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        <xsl:with-param name="referrer" select="$referrer"/>
        <xsl:with-param name="verbose" select="$verbose"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="xref-title">
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="object.title.markup"/>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<xsl:template match="d:author" mode="xref-title">
  <xsl:variable name="title">
    <xsl:call-template name="person.name"/>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<xsl:template match="d:authorgroup" mode="xref-title">
  <xsl:variable name="title">
    <xsl:call-template name="person.name.list"/>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<xsl:template match="d:cmdsynopsis" mode="xref-title">
  <xsl:variable name="title">
    <xsl:apply-templates select="(.//d:command)[1]" mode="xref"/>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<xsl:template match="d:funcsynopsis" mode="xref-title">
  <xsl:variable name="title">
    <xsl:apply-templates select="(.//d:function)[1]" mode="xref"/>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<xsl:template match="d:biblioentry|d:bibliomixed" mode="xref-title">
  <!-- handles both biblioentry and bibliomixed -->
  <xsl:variable name="title">
    <xsl:text>[</xsl:text>
    <xsl:choose>
      <xsl:when test="local-name(*[1]) = 'abbrev'">
        <xsl:apply-templates select="*[1]"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="(@id|@xml:id)[1]"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>]</xsl:text>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<xsl:template match="d:step" mode="xref-title">
  <xsl:call-template name="gentext">
    <xsl:with-param name="key" select="'Step'"/>
  </xsl:call-template>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="." mode="number"/>
</xsl:template>

<xsl:template match="d:step[not(./d:title)]" mode="title.markup">
  <xsl:call-template name="gentext">
    <xsl:with-param name="key" select="'Step'"/>
  </xsl:call-template>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="." mode="number"/>
</xsl:template>

<xsl:template match="d:co" mode="xref-title">
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="callout-bug"/>
  </xsl:variable>

  <xsl:value-of select="$title"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="d:link" name="link">
  <xsl:param name="linkend" select="@linkend"/>
  <xsl:param name="a.target"/>
  <xsl:param name="xhref" select="@xlink:href"/>

  <xsl:variable name="content">
    <xsl:call-template name="anchor"/>
    <xsl:choose>
      <xsl:when test="count(child::node()) &gt; 0">
        <!-- If it has content, use it -->
        <xsl:apply-templates/>
      </xsl:when>
      <!-- else look for an endterm -->
      <xsl:when test="@endterm">
        <xsl:variable name="etargets" select="key('id',@endterm)"/>
        <xsl:variable name="etarget" select="$etargets[1]"/>
        <xsl:choose>
          <xsl:when test="count($etarget) = 0">
            <xsl:message>
              <xsl:value-of select="count($etargets)"/>
              <xsl:text>Endterm points to nonexistent ID: </xsl:text>
              <xsl:value-of select="@endterm"/>
            </xsl:message>
            <xsl:text>???</xsl:text>
          </xsl:when>
          <xsl:otherwise>
              <xsl:apply-templates select="$etarget" mode="endterm"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <!-- Use the xlink:href if no other text -->
      <xsl:when test="@xlink:href">
        <xsl:value-of select="@xlink:href"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
          <xsl:text>Link element has no content and no Endterm. </xsl:text>
          <xsl:text>Nothing to show in the link to </xsl:text>
          <xsl:value-of select="(@xlink:href|@linkend)[1]"/>
        </xsl:message>
        <xsl:text>???</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:call-template name="simple.xlink">
    <xsl:with-param name="node" select="."/>
    <xsl:with-param name="linkend" select="$linkend"/>
    <xsl:with-param name="content" select="$content"/>
    <xsl:with-param name="a.target" select="$a.target"/>
    <xsl:with-param name="xhref" select="$xhref"/>
  </xsl:call-template>

</xsl:template>

<xsl:template match="d:ulink" name="ulink">
  <xsl:param name="url" select="@url"/>
  <xsl:variable name="link">
    <a>
      <xsl:apply-templates select="." mode="common.html.attributes"/>
      <xsl:if test="@id or @xml:id">
        <xsl:attribute name="id">
          <xsl:value-of select="(@id|@xml:id)[1]"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:attribute name="href"><xsl:value-of select="$url"/></xsl:attribute>
      <xsl:if test="$ulink.target != ''">
        <xsl:attribute name="target">
          <xsl:value-of select="$ulink.target"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="count(child::node())=0">
          <xsl:value-of select="$url"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates/>
        </xsl:otherwise>
      </xsl:choose>
    </a>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="function-available('suwl:unwrapLinks')">
      <xsl:copy-of select="suwl:unwrapLinks($link)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$link"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:olink" name="olink">
  <!-- olink content may be passed in from xlink olink -->
  <xsl:param name="content" select="NOTANELEMENT"/>

  <xsl:call-template name="anchor"/>

  <xsl:variable name="localinfo" select="@localinfo"/>

  <xsl:choose>
    <!-- olinks resolved by stylesheet and target database -->
    <xsl:when test="@targetdoc or @targetptr or                     (@xlink:role=$xolink.role and                      contains(@xlink:href, '#') )">

      <xsl:variable name="targetdoc.att">
        <xsl:choose>
          <xsl:when test="@targetdoc != ''">
            <xsl:value-of select="@targetdoc"/>
          </xsl:when>
          <xsl:when test="@xlink:role=$xolink.role and                        contains(@xlink:href, '#')">
            <xsl:value-of select="substring-before(@xlink:href, '#')"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="targetptr.att">
        <xsl:choose>
          <xsl:when test="@targetptr != ''">
            <xsl:value-of select="@targetptr"/>
          </xsl:when>
          <xsl:when test="@xlink:role=$xolink.role and                        contains(@xlink:href, '#')">
            <xsl:value-of select="substring-after(@xlink:href, '#')"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="olink.lang">
        <xsl:call-template name="l10n.language">
          <xsl:with-param name="xref-context" select="true()"/>
        </xsl:call-template>
      </xsl:variable>
    
      <xsl:variable name="target.database.filename">
        <xsl:call-template name="select.target.database">
          <xsl:with-param name="targetdoc.att" select="$targetdoc.att"/>
          <xsl:with-param name="targetptr.att" select="$targetptr.att"/>
          <xsl:with-param name="olink.lang" select="$olink.lang"/>
        </xsl:call-template>
      </xsl:variable>
    
      <xsl:variable name="target.database" select="document($target.database.filename,/)"/>
    
      <xsl:if test="$olink.debug != 0">
        <xsl:message>
          <xsl:text>Olink debug: root element of target.database '</xsl:text>
          <xsl:value-of select="$target.database.filename"/>
          <xsl:text>' is '</xsl:text>
          <xsl:value-of select="local-name($target.database/*[1])"/>
          <xsl:text>'.</xsl:text>
        </xsl:message>
      </xsl:if>
    
      <xsl:variable name="olink.key">
        <xsl:call-template name="select.olink.key">
          <xsl:with-param name="targetdoc.att" select="$targetdoc.att"/>
          <xsl:with-param name="targetptr.att" select="$targetptr.att"/>
          <xsl:with-param name="olink.lang" select="$olink.lang"/>
          <xsl:with-param name="target.database" select="$target.database"/>
        </xsl:call-template>
      </xsl:variable>
    
      <xsl:if test="string-length($olink.key) = 0">
        <xsl:message>
          <xsl:text>Error: unresolved olink: </xsl:text>
          <xsl:text>targetdoc/targetptr = '</xsl:text>
          <xsl:value-of select="$targetdoc.att"/>
          <xsl:text>/</xsl:text>
          <xsl:value-of select="$targetptr.att"/>
          <xsl:text>'.</xsl:text>
        </xsl:message>
      </xsl:if>

      <xsl:variable name="href">
        <xsl:call-template name="make.olink.href">
          <xsl:with-param name="olink.key" select="$olink.key"/>
          <xsl:with-param name="target.database" select="$target.database"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="hottext">
        <xsl:choose>
          <xsl:when test="string-length($content) != 0">
            <xsl:copy-of select="$content"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="olink.hottext">
              <xsl:with-param name="olink.key" select="$olink.key"/>
              <xsl:with-param name="olink.lang" select="$olink.lang"/>
              <xsl:with-param name="target.database" select="$target.database"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="olink.docname.citation">
        <xsl:call-template name="olink.document.citation">
          <xsl:with-param name="olink.key" select="$olink.key"/>
          <xsl:with-param name="target.database" select="$target.database"/>
          <xsl:with-param name="olink.lang" select="$olink.lang"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="olink.page.citation">
        <xsl:call-template name="olink.page.citation">
          <xsl:with-param name="olink.key" select="$olink.key"/>
          <xsl:with-param name="target.database" select="$target.database"/>
          <xsl:with-param name="olink.lang" select="$olink.lang"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="$href != ''">
          <a href="{$href}">
            <xsl:apply-templates select="." mode="common.html.attributes"/>
            <xsl:copy-of select="$hottext"/>
          </a>
          <xsl:copy-of select="$olink.page.citation"/>
          <xsl:copy-of select="$olink.docname.citation"/>
        </xsl:when>
        <xsl:otherwise>
          <span class="olink"><xsl:copy-of select="$hottext"/></span>
          <xsl:copy-of select="$olink.page.citation"/>
          <xsl:copy-of select="$olink.docname.citation"/>
        </xsl:otherwise>
      </xsl:choose>

    </xsl:when>

    <!-- Or use old olink mechanism -->
    <xsl:otherwise>
      <xsl:variable name="href">
        <xsl:choose>
          <xsl:when test="@linkmode">
            <!-- use the linkmode to get the base URI, use localinfo as fragid -->
            <xsl:variable name="modespec" select="key('id',@linkmode)"/>
            <xsl:if test="count($modespec) != 1                           or local-name($modespec) != 'modespec'">
              <xsl:message>Warning: olink linkmode pointer is wrong.</xsl:message>
            </xsl:if>
            <xsl:value-of select="$modespec"/>
            <xsl:if test="@localinfo">
              <xsl:text>#</xsl:text>
              <xsl:value-of select="@localinfo"/>
            </xsl:if>
          </xsl:when>
          <xsl:when test="@type = 'href'">
            <xsl:call-template name="olink.outline">
              <xsl:with-param name="outline.base.uri" select="unparsed-entity-uri(@targetdocent)"/>
              <xsl:with-param name="localinfo" select="@localinfo"/>
              <xsl:with-param name="return" select="'href'"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$olink.resolver"/>
            <xsl:text>?</xsl:text>
            <xsl:value-of select="$olink.sysid"/>
            <xsl:value-of select="unparsed-entity-uri(@targetdocent)"/>
            <!-- XSL gives no access to the public identifier (grumble...) -->
            <xsl:if test="@localinfo">
              <xsl:text>&amp;</xsl:text>
              <xsl:value-of select="$olink.fragid"/>
              <xsl:value-of select="@localinfo"/>
            </xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
    
      <xsl:choose>
        <xsl:when test="$href != ''">
          <a href="{$href}">
            <xsl:apply-templates select="." mode="common.html.attributes"/>
            <xsl:call-template name="olink.hottext"/>
          </a>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="olink.hottext"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="*" mode="pagenumber.markup">
  <!-- no-op in HTML -->
</xsl:template>


<xsl:template name="olink.outline">
  <xsl:param name="outline.base.uri"/>
  <xsl:param name="localinfo"/>
  <xsl:param name="return" select="d:href"/>

  <xsl:variable name="outline-file" select="concat($outline.base.uri,                                $olink.outline.ext)"/>

  <xsl:variable name="outline" select="document($outline-file,.)/div"/>

  <xsl:variable name="node-href">
    <xsl:choose>
      <xsl:when test="$localinfo != ''">
        <xsl:variable name="node" select="$outline//                                    *[@id=$localinfo or @xml:id=$localinfo]"/>
        <xsl:value-of select="$node/@href"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$outline/@href"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="node-xref">
    <xsl:choose>
      <xsl:when test="$localinfo != ''">
        <xsl:variable name="node" select="$outline//                                *[@id=$localinfo or @xml:id=$localinfo]"/>
        <xsl:copy-of select="$node/d:xref"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$outline/d:xref"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$return = 'href'">
      <xsl:value-of select="$node-href"/>
    </xsl:when>
    <xsl:when test="$return = 'xref'">
      <xsl:value-of select="$node-xref"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$node-xref"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="xref.xreflabel">
  <!-- called to process an xreflabel...you might use this to make  -->
  <!-- xreflabels come out in the right font for different targets, -->
  <!-- for example. -->
  <xsl:param name="target" select="."/>
  <xsl:value-of select="$target/@xreflabel"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="d:title" mode="xref">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="d:command" mode="xref">
  <xsl:call-template name="inline.boldseq"/>
</xsl:template>

<xsl:template match="d:function" mode="xref">
  <xsl:call-template name="inline.monoseq"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="insert.title.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="title"/>

  <xsl:choose>
    <!-- FIXME: what about the case where titleabbrev is inside the info? -->
    <xsl:when test="$purpose = 'xref' and d:titleabbrev">
      <xsl:apply-templates select="." mode="titleabbrev.markup"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$title"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="d:chapter|d:appendix" mode="insert.title.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="title"/>

  <xsl:choose>
    <xsl:when test="$purpose = 'xref'">
      <em xmlns:xslo="http://www.w3.org/1999/XSL/Transform">
        <xsl:copy-of select="$title"/>
      </em>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$title"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="*" mode="insert.subtitle.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="subtitle"/>

  <xsl:copy-of select="$subtitle"/>
</xsl:template>

<xsl:template match="*" mode="insert.label.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="label"/>

  <xsl:copy-of select="$label"/>
</xsl:template>

<xsl:template match="*" mode="insert.pagenumber.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="pagenumber"/>

  <xsl:copy-of select="$pagenumber"/>
</xsl:template>

<xsl:template match="*" mode="insert.direction.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="direction"/>

  <xsl:copy-of select="$direction"/>
</xsl:template>

<xsl:template match="*" mode="insert.olink.docname.markup">
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="docname"/>

  <span class="olinkdocname">
    <xsl:copy-of select="$docname"/>
  </span>

</xsl:template>

</xsl:stylesheet>
