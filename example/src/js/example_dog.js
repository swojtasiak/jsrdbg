debugger;
print('Hello. ');
(function(fn) {
    print('Yes, ');
    fn('this is dog.');
})(function(msg) {
    print(msg);
});
print('Woof, woof.');
debugger;

