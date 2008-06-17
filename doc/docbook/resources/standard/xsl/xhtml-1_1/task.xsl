<?xml version="1.0" encoding="ASCII"?>
<!--This file was created automatically by html2xhtml-->
<!--from the HTML stylesheets.-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">

<!-- ********************************************************************
     $Id: task.xsl 6910 2007-06-28 23:23:30Z xmldoc $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="task">
  <xsl:variable name="param.placement" select="substring-after(normalize-space($formal.title.placement),                                         concat(local-name(.), ' '))"/>

  <xsl:variable name="placement">
    <xsl:choose>
      <xsl:when test="contains($param.placement, ' ')">
        <xsl:value-of select="substring-before($param.placement, ' ')"/>
      </xsl:when>
      <xsl:when test="$param.placement = ''">before</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$param.placement"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="preamble" select="*[not(self::title                               or self::titleabbrev)]"/>

  <div>
    <xsl:apply-templates select="." mode="class.attribute"/>
    <xsl:call-template name="anchor"/>

    <xsl:if test="title and $placement = 'before'">
      <xsl:call-template name="formal.object.heading"/>
    </xsl:if>

    <xsl:apply-templates select="$preamble"/>

    <xsl:if test="title and $placement != 'before'">
      <xsl:call-template name="formal.object.heading"/>
    </xsl:if>
  </div>
</xsl:template>

<xsl:template match="task/title">
  <!-- nop -->
</xsl:template>

<xsl:template match="tasksummary">
  <xsl:call-template name="semiformal.object"/>
</xsl:template>

<xsl:template match="tasksummary/title"/>

<xsl:template match="taskprerequisites">
  <xsl:call-template name="semiformal.object"/>
</xsl:template>

<xsl:template match="taskprerequisites/title"/>

<xsl:template match="taskrelated">
  <xsl:call-template name="semiformal.object"/>
</xsl:template>

<xsl:template match="taskrelated/title"/>

</xsl:stylesheet>
