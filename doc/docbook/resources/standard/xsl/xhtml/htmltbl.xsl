<?xml version="1.0" encoding="ASCII"?>
<!--This file was created automatically by html2xhtml-->
<!--from the HTML stylesheets.-->
<xsl:stylesheet exclude-result-prefixes="d"
                 xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:d="http://docbook.org/ns/docbook"
xmlns="http://www.w3.org/1999/xhtml" version="1.0">

<!-- ********************************************************************
     $Id: htmltbl.xsl 8477 2009-07-13 11:38:55Z nwalsh $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="d:colgroup" mode="htmlTable">
  <xsl:element name="{local-name()}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:apply-templates select="@*" mode="htmlTableAtt"/>
    <xsl:apply-templates mode="htmlTable"/>
  </xsl:element>
</xsl:template>

<xsl:template match="d:col" mode="htmlTable">
  <xsl:element name="{local-name()}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:apply-templates select="@*" mode="htmlTableAtt"/>
  </xsl:element>
</xsl:template>

<xsl:template match="d:caption" mode="htmlTable">
  <!-- do not use xsl:copy because of XHTML's needs -->
  <caption>  
    <xsl:apply-templates select="@*" mode="htmlTableAtt"/>

    <xsl:apply-templates select=".." mode="object.title.markup">
      <xsl:with-param name="allow-anchors" select="1"/>
    </xsl:apply-templates>

  </caption>
</xsl:template>

<xsl:template match="d:tbody|d:thead|d:tfoot|d:tr" mode="htmlTable">
  <xsl:element name="{local-name(.)}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:apply-templates select="@*" mode="htmlTableAtt"/>
    <xsl:apply-templates mode="htmlTable"/>
  </xsl:element>
</xsl:template>

<xsl:template match="d:th|d:td" mode="htmlTable">
  <xsl:element name="{local-name(.)}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:apply-templates select="@*" mode="htmlTableAtt"/>
    <xsl:apply-templates/> <!-- *not* mode=htmlTable -->
  </xsl:element>
</xsl:template>

<!-- don't copy through DocBook-specific attributes on HTML table markup -->
<!-- default behavior is to not copy through because there are more
     DocBook attributes than HTML attributes -->
<xsl:template mode="htmlTableAtt" match="@*"/>

<!-- copy these through -->
<xsl:template mode="htmlTableAtt" match="@abbr                    | @align                    | @axis                    | @bgcolor                    | @border                    | @cellpadding                    | @cellspacing                    | @char                    | @charoff                    | @class                    | @dir                    | @frame                    | @headers                    | @height                    | @lang                    | @nowrap                    | @onclick                    | @ondblclick                    | @onkeydown                    | @onkeypress                    | @onkeyup                    | @onmousedown                    | @onmousemove                    | @onmouseout                    | @onmouseover                    | @onmouseup                    | @rules                    | @style                    | @summary                    | @title                    | @valign                    | @valign                    | @width                    | @xml:lang">
  <xsl:copy-of select="."/>
</xsl:template>

<xsl:template match="@span|@rowspan|@colspan" mode="htmlTableAtt">
  <!-- No need to copy through the DTD's default value "1" of the attribute -->
  <xsl:if test="number(.) != 1">
    <xsl:attribute name="{local-name(.)}">
      <xsl:value-of select="."/>
    </xsl:attribute>
  </xsl:if>
</xsl:template>

<!-- map floatstyle to HTML float values -->
<xsl:template match="@floatstyle" mode="htmlTableAtt">
  <xsl:attribute name="style">
    <xsl:text>float: </xsl:text>
    <xsl:choose>
      <xsl:when test="contains(., 'left')">left</xsl:when>
      <xsl:when test="contains(., 'right')">right</xsl:when>
      <xsl:when test="contains(., 'start')">
        <xsl:value-of select="$direction.align.start"/>
      </xsl:when>
      <xsl:when test="contains(., 'end')">
        <xsl:value-of select="$direction.align.end"/>
      </xsl:when>
      <xsl:when test="contains(., 'inside')">
        <xsl:value-of select="$direction.align.start"/>
      </xsl:when>
      <xsl:when test="contains(., 'outside')">
        <xsl:value-of select="$direction.align.end"/>
      </xsl:when>
      <xsl:when test="contains(., 'before')">none</xsl:when>
      <xsl:when test="contains(., 'none')">none</xsl:when>
    </xsl:choose>
    <xsl:text>;</xsl:text>
  </xsl:attribute>
</xsl:template>

</xsl:stylesheet>
