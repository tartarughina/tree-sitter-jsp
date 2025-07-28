<%-- Test EL expressions in script and style tags --%>

<script>
var userId = ${userId};
var userName = "${userName}";
if (${isLoggedIn}) {
  console.log("User: ${user.name}");
}
</script>

<style>
.user-panel {
  width: ${panelWidth}px;
  height: ${panelHeight}px;
  background-color: ${theme.primaryColor};
}
.dynamic-class-${status} {
  display: ${isVisible ? 'block' : 'none'};
}
</style>

<script type="text/javascript">
function processData() {
  var data = {
    id: ${item.id},
    name: "${item.name}",
    active: ${item.active}
  };
  return data;
}
</script>
