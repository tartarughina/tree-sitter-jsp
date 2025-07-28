<%-- Test various EL expression patterns from Oracle spec --%>

<%-- Simple variable --%>
${name}

<%-- Nested property --%>
${name.foo.bar}

<%-- In static text --%>
Hello ${user.name}, welcome to ${site.title}!

<%-- In tag attributes - single expression --%>
<c:if test="${sessionScope.cart.numberOfItems > 0}">
  Cart has items
</c:if>

<%-- In tag attributes - mixed with text --%>
<some:tag value="prefix${expr1}${expr2}suffix${expr3}"/>

<%-- Complex expressions with operators --%>
${bean1.a < 3}
${1.2E4 + 1.4}
${condition ? value1 : value2}

<%-- Array/Map access --%>
${items[0]}
${map['key']}
${user.addresses[0].street}

<%-- Function calls --%>
${f:equals(selectedLocale, currentLocale)}

<%-- Escaped EL (should not be parsed as EL) --%>
<mytags:example attr1="an expression is ${'${'}true}" />

<%-- Nested braces --%>
${users.stream().filter(u -> u.age > 18).count()}
