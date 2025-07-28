<%-- Oracle EL Specification Compliance Test --%>

<%-- 1. Single expression in attribute --%>
<some:tag value="${expr}"/>

<%-- 2. Multiple expressions with text in attribute --%>
<some:tag value="some${expr}${expr}text${expr}"/>

<%-- 3. Text only in attribute --%>
<some:tag value="sometext"/>

<%-- 4. Complex expressions --%>
<c:if test="${sessionScope.cart.numberOfItems > 0}">
  Cart has items
</c:if>

<%-- 5. Nested property access --%>
<div data-street="${user.addresses[0].street}">Address</div>

<%-- 6. Conditional expressions --%>
<div class="${condition ? 'active' : 'inactive'}">Status</div>

<%-- 7. Function calls --%>
<div data-equal="${f:equals(selectedLocale, currentLocale)}">Comparison</div>

<%-- 8. Escaped EL (should be treated as literal text) --%>
<mytags:example attr1="an expression is ${'${'}true}" />

<%-- 9. Mixed quotes --%>
<div class='${singleQuote}' title="${doubleQuote}">Mixed</div>
