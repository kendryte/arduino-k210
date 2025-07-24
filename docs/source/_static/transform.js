document.addEventListener("DOMContentLoaded", function () {
  console.log("给页面添加动画");
  // 给页面添加动画
  const content = document.querySelector(".bd-article-container");
  if (content) {
    content.classList.remove("page-transition", "page-enter-active");
    void content.offsetWidth;
    content.classList.add("page-transition");
    setTimeout(() => {
      content.classList.add("page-enter-active");
    }, 10);
  }
});
