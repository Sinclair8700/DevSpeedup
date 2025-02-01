var gulp        = require('gulp'),
    sourcemaps  = require('gulp-sourcemaps'),
    concat      = require('gulp-concat'),
    uglify      = require('gulp-uglify'),
    replace     = require('gulp-replace'),
    fs          = require('fs'),
    postcss     = require('gulp-postcss'),
    cssnext     = require('postcss-cssnext'),
    clipboard   = require("gulp-clipboard"),
    cssnano     = require('gulp-cssnano'),
    strip       = require('gulp-strip-comments'),
    babel       = require('gulp-babel'),
    insert      = require('gulp-insert'),
    prettier = require('gulp-prettier');


gulp.task('default', ['watch']);

/**
 * Wrap gulp streams into fail-safe function for better error reporting
 */
function wrapPipe(taskFn) {
  return function(done) {
    var onSuccess = function() {
      done();
    };
    var onError = function(err) {
      done(err);
    }
    var outStream = taskFn(onSuccess, onError);
    if(outStream && typeof outStream.on === 'function') {
      outStream.on('end', onSuccess);
    }
  }
}

/**
 * Build SCSS using Post CSS
 */
gulp.task('build-css', wrapPipe(function(success, error) {
  return gulp.src('source/css/test.css')
    .pipe( postcss([
      require('precss'),
      cssnext({browsers: ['last 2 version']}),
    ])).on('error', error)
    .pipe(cssnano({zindex: false})).on('error', error)
    .pipe( sourcemaps.write('.') ).on('error', error)
    .pipe( gulp.dest('dist') ).on('error', error);
}));

/**
 * Build SCSS Code Review using Post CSS
 */
gulp.task('build-code-review-css', wrapPipe(function(success, error) {
  return gulp.src('source/css/test.css')
    .pipe( postcss([
      require('precss')
    ])).on('error', error)
    .pipe( sourcemaps.write('.') ).on('error', error)
    .pipe( gulp.dest('code-review') ).on('error', error);
}));

/**
 * Build javascript & insert css into build
 */
gulp.task('build-js', ['build-css'], wrapPipe(function(success, error) {
  return gulp.src('source/javascript/test.js')
    .pipe(replace(/%CSS%/, function(s) {
        var style = fs.readFileSync('dist/test.css', 'utf8');
        return style;
    })).on('error', error)
    .pipe(replace(/%BN%/, function(s) {
        return Date.now();
    })).on('error', error)
    .pipe(sourcemaps.init())
      .pipe(concat('test.js')).on('error', error)
      .pipe(babel({
        presets: ["@babel/preset-env"]
      })).on('error', error)
      .pipe(uglify({ keep_fnames: true })).on('error', error)
      .pipe(strip()).on('error', error)
      .pipe(insert.transform(function(contents, file) {
        var conf = fs.readFileSync('source/javascript/config.js', 'utf8');
        return contents + "\n" + conf
      })).on('error', error)
      .pipe(clipboard()).on('error', error)
    .pipe(gulp.dest('dist'));
}));

/**
 * Output for code review
 */
gulp.task('build-code-review-js', ['build-code-review-css'], wrapPipe(function(success, error) {
  return gulp.src('source/javascript/test.js')
    .pipe(replace(/%CSS%/, function(s) {
        var style = fs.readFileSync('dist/test.css', 'utf8');
        return style;
    })).on('error', error)
    .pipe(replace(/%BN%/, function(s) {
        return Date.now();
    })).on('error', error)
    .pipe(sourcemaps.init())
      .pipe(concat('review.js')).on('error', error)
      .pipe(insert.transform(function(contents, file) {
        var conf = fs.readFileSync('source/javascript/config.js', 'utf8');
        return contents + "\n" + conf
      })).on('error', error)
    .pipe(gulp.dest('code-review'));
}));

gulp.task('watch', function() {
  gulp.watch('source/javascript/**/*.js', ['build-js']);
  gulp.watch('source/css/**/*.css', ['build-js']);
  gulp.watch('source/javascript/**/*.js', ['build-code-review-js']);
  gulp.watch('source/css/**/*.css', ['build-code-review-js']);
});