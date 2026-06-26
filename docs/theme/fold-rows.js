// Make linkless folder rows in the sidebar fold/unfold when the whole row is
// clicked, like the chevron does. mdbook renders these titles as a bare <span>
// (not an <a>), so they're inert by default. Delegated so it works regardless of
// when the custom-element sidebar hydrates.
document.addEventListener('click', function (ev) {
    // Real links and the chevron (<a class="chapter-fold-toggle">) keep their
    // own behavior - mdbook already folds on the chevron.
    if (ev.target.closest('a')) return;
    var wrapper = ev.target.closest('.chapter-link-wrapper');
    if (!wrapper || !wrapper.querySelector('.chapter-fold-toggle')) return;
    var li = wrapper.closest('li.chapter-item');
    if (li) li.classList.toggle('expanded');
});
