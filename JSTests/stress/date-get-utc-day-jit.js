function shouldBe(actual, expected) {
    if (actual !== expected)
        throw new Error('bad value: ' + actual);
}

function test(date) {
    return date.getUTCDay();
}
noInline(test);

var date = new Date();
var invalid = new Date(NaN);
var expected = date.getUTCDay();
for (var i = 0; i < testLoopCount; ++i) {
    shouldBe(test(date), expected);
    var d = new Date();
    shouldBe(test(d), d.getUTCDay());
    shouldBe(isNaN(test(invalid)), true);
}
