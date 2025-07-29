<c:if test=${layoutValues.getNumOfImagesToDisplay() > 0}>
  <%-- TODO: figure out how to properly use the aui scroller property=horizontal --%>
  <div class="cr-ajax-carousel-margin cr-scroller-content-end">
    <m:include component="${carouselImageListView}" />
  </div>
</c:if>
