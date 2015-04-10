(function level_0() {
    var l_0_1 = 0;
    var l_0_2 = 'test0';
    var l_0_3 = { info: 'test0' };
    (function level_1() {
        var l_1_1 = 1;
        var l_1_2 = 'test1';
        var l_1_3 = { info: 'test1' };
        (function level_2() {
            var l_2_1 = l_1_1;
            var l_2_2 = l_1_2;
            var l_2_3 = l_1_3;
            (function level_3() {
                var l_3_1 = 3;
                var l_3_2 = 'test3';
                var l_3_3 = { info: 'test3' };
                (function level_4() {
                    var l_4_1 = 4;
                    var l_4_2 = 'test4';
                    var l_4_3 = { info: 'test4' };
                    (function level_5() {
                        var l_5_1 = 5;
                        var l_5_2 = 'test5';
                        var l_5_3 = { info: 'test5' };
                        debugger;
                        return l_5_1;
                    })();
                })();
            })();
        })();
    })();
})();

