<%-- Test EL in various attribute contexts --%>

<%-- Single EL in attribute --%>
<div class="${userClass}">Content</div>

<%-- Multiple EL in attribute --%>
<img src="${baseUrl}/images/${imageId}.jpg" alt="${imageAlt}"/>

<%-- Mixed text and EL in attribute --%>
<div class="prefix-${status}-suffix">Content</div>

<%-- Complex EL expressions in attributes --%>
<div data-count="${items.size()}" data-empty="${items.empty ? 'true' : 'false'}">
  Content
</div>

<%-- EL in different quote types --%>
<div class='${singleQuoteClass}' title="${doubleQuoteTitle}">
  Content
</div>
