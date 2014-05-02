<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- This is a modified version of the Google style guide. The original is here:
http://google-styleguide.googlecode.com/svn/trunk/styleguide.xsl
It is licensed under the CC-By 3.0 License.

This version has been modified by cutting out several items, and by changing the
formatting of others. If you want fuller functionality, use the original. -->

  <xsl:output method="html"/>
  <!-- Set to 1 to show explanations by default.  Set to 0 to hide them -->
  <xsl:variable name="show_explanation_default" select="0" />
  <!-- The characters within the Webdings font that show the triangles -->
  <xsl:variable name="show_button_text" select="'&#x25B6;'" />
  <xsl:variable name="hide_button_text" select="'&#x25BD;'" />
  <!-- The suffix for names -->
  <xsl:variable name="button_suffix" select="'__button'"/>
  <xsl:variable name="body_suffix" select="'__body'"/>
  <!-- For easy reference, the name of the button -->
  <xsl:variable name="show_hide_all_button" select="'show_hide_all_button'"/>

  <!-- The top-level element -->
  <xsl:template match="GUIDE">
      <HTML>
          <HEAD>
              <TITLE><xsl:value-of select="@title"/></TITLE>
              <META http-equiv="Content-Type" content="text/html; charset=utf-8"/>
              <LINK HREF="styleguide.css"
                    type="text/css" rel="stylesheet"/>

              <SCRIPT language="javascript" type="text/javascript">

                function GetElementsByName(name) {
                  // Workaround a bug on old versions of opera.
                  if (document.getElementsByName) {
                    return document.getElementsByName(name);
                  } else {
                    return [document.getElementById(name)];
                  }
                }

                /**
                 * @param {string} namePrefix The prefix of the body name.
                 * @param {function(boolean): boolean} getVisibility Computes the new
                 *     visibility state, given the current one.
                 */
                function ChangeVisibility(namePrefix, getVisibility) {
                  var bodyName = namePrefix + '<xsl:value-of select="$body_suffix"/>';
                  var buttonName = namePrefix + '<xsl:value-of select="$button_suffix"/>';
                  var bodyElements = GetElementsByName(bodyName);
                  if (bodyElements.length != 1) {
                    throw Error('ShowHideByName() got the wrong number of bodyElements:  ' + 
                        bodyElements.length);
                  } else {
                    var bodyElement = bodyElements[0];
                    var buttonElement = GetElementsByName(buttonName)[0];
                    var isVisible = bodyElement.style.display != "none";
                    if (getVisibility(isVisible)) {
                      bodyElement.style.display = "inline";
                      buttonElement.innerHTML = '<xsl:value-of select="$hide_button_text"/>';
                    } else {
                      bodyElement.style.display = "none";
                      buttonElement.innerHTML = '<xsl:value-of select="$show_button_text"/>';
                    }
                  }
                }

                function ShowHideByName(namePrefix) {
                  ChangeVisibility(namePrefix, function(old) { return !old; });
                }

                function ShowByName(namePrefix) {
                  ChangeVisibility(namePrefix, function() { return true; });
                }

                function ShowHideAll() {
                  var allButton = GetElementsByName("show_hide_all_button")[0];
                  if (allButton.innerHTML == '<xsl:value-of select="$hide_button_text"/>') {
                    allButton.innerHTML = '<xsl:value-of select="$show_button_text"/>';
                    SetHiddenState(document.getElementsByTagName("body")[0].childNodes, "none", '<xsl:value-of select="$show_button_text"/>');
                  } else {
                    allButton.innerHTML = '<xsl:value-of select="$hide_button_text"/>';
                    SetHiddenState(document.getElementsByTagName("body")[0].childNodes, "inline", '<xsl:value-of select="$hide_button_text"/>');
                  }
                }

                // Recursively sets state of all children
                // of a particular node.
                function SetHiddenState(root, newState, newButton) {
                  for (var i = 0; i != root.length; i++) {
                    SetHiddenState(root[i].childNodes, newState, newButton);
                    if (root[i].className == 'showhide_button')  {
                      root[i].innerHTML = newButton;
                    }
                    if (root[i].className == 'stylepoint_body' ||
                        root[i].className == 'link_button')  {
                      root[i].style.display = newState;
                    }
                  }
                }


                function EndsWith(str, suffix) {
                  var l = str.length - suffix.length;
                  return l >= 0 &amp;&amp; str.indexOf(suffix, l) == l;
                }

                function RefreshVisibilityFromHashParam() {
                  var hashRegexp = new RegExp('#([^&amp;#]*)$');
                  var hashMatch = hashRegexp.exec(window.location.href);
                  var anchor = hashMatch &amp;&amp; GetElementsByName(hashMatch[1])[0];
                  var node = anchor;
                  var suffix = '<xsl:value-of select="$body_suffix"/>';
                  while (node) {
                    var id = node.id;
                    var matched = id &amp;&amp; EndsWith(id, suffix);
                    if (matched) {
                      var len = id.length - suffix.length;
                      ShowByName(id.substring(0, len));
                      if (anchor.scrollIntoView) {
                        anchor.scrollIntoView();
                      }

                      return;
                    }
                    node = node.parentNode;
                  }
                }

                window.onhashchange = RefreshVisibilityFromHashParam;

                window.onload = function() {
                  // if the URL contains "?showall=y", expand the details of all children
                  var showHideAllRegex = new RegExp("[\\?&amp;](showall)=([^&amp;#]*)");
                  var showHideAllValue = showHideAllRegex.exec(window.location.href);
                  if (showHideAllValue != null) {
                    if (showHideAllValue[2] == "y") {
                      SetHiddenState(document.getElementsByTagName("body")[0].childNodes, 
                          "inline", '<xsl:value-of select="$hide_button_text"/>');
                    } else {
                      SetHiddenState(document.getElementsByTagName("body")[0].childNodes, 
                          "none", '<xsl:value-of select="$show_button_text"/>');
                    }
                  }
                  var showOneRegex = new RegExp("[\\?&amp;](showone)=([^&amp;#]*)");
                  var showOneValue = showOneRegex.exec(window.location.href);
                  if (showOneValue) {
                    ShowHideByName(showOneValue[2]);
                  }


                  RefreshVisibilityFromHashParam();
                }
              </SCRIPT>
          </HEAD>
          <BODY>
            <H1><xsl:value-of select="@title"/></H1>
              <xsl:apply-templates/>
          </BODY>
      </HTML>
  </xsl:template>

  <xsl:template match="OVERVIEW">
    <xsl:variable name="button_text">
      <xsl:choose>
        <xsl:when test="$show_explanation_default">
          <xsl:value-of select="$hide_button_text"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$show_button_text"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <DIV style="margin-left: 30%; margin-right: 30%; margin-top: 100px; text-align: center; font-size: 75%;">
      <P>
        Each style point has a summary for which additional information is available
        by toggling the accompanying arrow button. You may toggle all summaries with the big arrow button:
      </P>
      <DIV style=" font-size: larger;">
        <SPAN class="showhide_button" style="font-size: 180%; float: none">
          <xsl:attribute name="onclick"><xsl:value-of select="'javascript:ShowHideAll()'"/></xsl:attribute>
          <xsl:attribute name="name"><xsl:value-of select="$show_hide_all_button"/></xsl:attribute>
          <xsl:attribute name="id"><xsl:value-of select="$show_hide_all_button"/></xsl:attribute>
          <xsl:value-of select="$button_text"/>
        </SPAN>
      </DIV>
    </DIV>
    <xsl:call-template name="TOC">
      <xsl:with-param name="root" select=".."/>
    </xsl:call-template>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="CATEGORY">
    <DIV>
      <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
      <H2>
        <xsl:variable name="category_name">
          <xsl:call-template name="anchorname">
            <xsl:with-param name="sectionname" select="@title"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:attribute name="name"><xsl:value-of select="$category_name"/></xsl:attribute>
        <xsl:attribute name="id"><xsl:value-of select="$category_name"/></xsl:attribute>
        <xsl:value-of select="@title"/>
      </H2>
      <xsl:apply-templates/>
    </DIV>
  </xsl:template>

  <xsl:template match="STYLEPOINT">
    <DIV>
      <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
      <xsl:variable name="stylepoint_name">
        <xsl:call-template name="anchorname">
          <xsl:with-param name="sectionname" select="@title"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="button_text">
        <xsl:choose>
          <xsl:when test="$show_explanation_default">
            <xsl:value-of select="$hide_button_text"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$show_button_text"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <H3>
        <A>
          <xsl:attribute name="name"><xsl:value-of select="$stylepoint_name"/></xsl:attribute>
          <xsl:attribute name="id"><xsl:value-of select="$stylepoint_name"/></xsl:attribute>
          <xsl:value-of select="@title"/>
        </A>
      </H3>
      <xsl:variable name="buttonName">
        <xsl:value-of select="$stylepoint_name"/><xsl:value-of select="$button_suffix"/>
      </xsl:variable>
      <xsl:variable name="onclick_definition">
        <xsl:text>javascript:ShowHideByName('</xsl:text>
        <xsl:value-of select="$stylepoint_name"/>
        <xsl:text>')</xsl:text>
      </xsl:variable>
      <SPAN class="showhide_button">
        <xsl:attribute name="onclick"><xsl:value-of select="$onclick_definition"/></xsl:attribute>
        <xsl:attribute name="name"><xsl:value-of select="$buttonName"/></xsl:attribute>
        <xsl:attribute name="id"><xsl:value-of select="$buttonName"/></xsl:attribute>
        <xsl:value-of select="$button_text"/>
      </SPAN>
      <xsl:apply-templates>
        <xsl:with-param name="anchor_prefix" select="$stylepoint_name" />
      </xsl:apply-templates>
    </DIV>
  </xsl:template>

  <xsl:template match="SUMMARY">
    <xsl:param name="anchor_prefix" />
    <DIV style="display:inline;">
      <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
      <xsl:apply-templates/>
    </DIV>
  </xsl:template>

  <xsl:template match="BODY">
    <xsl:param name="anchor_prefix" />
    <DIV>
      <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
      <DIV class="stylepoint_body">
        <xsl:attribute name="name"><xsl:value-of select="$anchor_prefix"/><xsl:value-of select="$body_suffix"/></xsl:attribute>
        <xsl:attribute name="id"><xsl:value-of select="$anchor_prefix"/><xsl:value-of select="$body_suffix"/></xsl:attribute>
        <xsl:attribute name="style">
          <xsl:choose>
            <xsl:when test="$show_explanation_default">display: inline</xsl:when>
            <xsl:otherwise>display: none</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:apply-templates/>
      </DIV>
    </DIV>
  </xsl:template>

  <xsl:template match="CODE_SNIPPET">
    <DIV>
      <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
      <PRE><xsl:apply-templates/></PRE>
    </DIV>
  </xsl:template>

  <xsl:template match="BAD_CODE_SNIPPET">
    <DIV>
      <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
      <PRE class="badcode"><xsl:apply-templates/></PRE>
    </DIV>
  </xsl:template>

  <!-- This passes through any HTML elements that the
    XML doc uses for minor formatting -->
  <xsl:template match="a|address|blockquote|br|center|cite|code|dd|div|dl|dt|em|hr|i|img|li|ol|p|pre|span|table|td|th|tr|ul|var|A|ADDRESS|BLOCKQUOTE|BR|CENTER|CITE|CODE|DD|DIV|DL|DT|EM|HR|I|LI|OL|P|PRE|SPAN|TABLE|TD|TH|TR|UL|VAR">
      <xsl:element name="{local-name()}">
          <xsl:copy-of select="@*"/>
          <xsl:apply-templates/>
      </xsl:element>
  </xsl:template>

    <!-- Builds the table of contents -->
  <xsl:template name="TOC">
    <xsl:param name="root"/>
    <DIV class="toc">
      <H2>Table of Contents</H2>
      <ul>
      <xsl:for-each select="$root/CATEGORY">
        <li>
          <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
            <A>
              <xsl:attribute name="href">
                <xsl:text>#</xsl:text>
                <xsl:call-template name="anchorname">
                  <xsl:with-param name="sectionname" select="@title"/>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:value-of select="@title"/>
            </A>
        </li>
      </xsl:for-each>
      </ul>
    </DIV>
  </xsl:template>

  <xsl:template name="TOC_one_stylepoint">
    <xsl:param name="stylepoint"/>
  </xsl:template>

  <!-- Creates a standard anchor given any text.
       Substitutes underscore for characters unsuitable for URLs  -->
  <xsl:template name="anchorname">
    <xsl:param name="sectionname"/>
    <!-- strange quoting necessary to strip apostrophes -->
    <xsl:variable name="bad_characters" select="&quot; ()#'&quot;"/>
    <xsl:value-of select="translate($sectionname,$bad_characters,'_____')"/>
  </xsl:template>
</xsl:stylesheet>

