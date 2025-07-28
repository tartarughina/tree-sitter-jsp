<%-- EL in different contexts --%>

<%-- 1. EL in static text (should work) --%>
Hello ${name}!

<%-- 2. EL in attribute value (currently broken) --%>
<div class="user-${status}">Content</div>

<%-- 3. Multiple EL in attribute (currently broken) --%>
<img src="${baseUrl}/images/${imageId}.jpg" alt="${imageAlt}"/>

<%-- 4. EL with complex expressions --%>
${user.addresses[0].street}
${items.size() > 0 ? 'has-items' : 'empty'}
