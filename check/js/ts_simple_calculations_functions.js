debugger;

function Utils() {
}

Utils.sum = function(x, y) {
    return x+y;
};

function Manager() {
}

Manager.prototype = {
    calculate: function( x ) {
        return (function() {
            return {
                result: this.show(Utils.sum(10, x))
            };
        }).call(this);
    },
    show: function(x) {
        return x;
    }
};

var manager = new Manager();

manager.calculate(5);

