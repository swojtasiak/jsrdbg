
module.exports = function(grunt) {

    'use strict';

    require('load-grunt-tasks')(grunt);

    require('time-grunt')(grunt);

    grunt.initConfig( {

        jshint: {
            options: {
                jshintrc: true,
                reporter: require('jshint-stylish')
            },
            all: {
                src: [
                    '*.js'
                ]
            }
        },

        watch: {
            options: {
                cwd: '.'
            },
            gruntfile: {
                files: ['Gruntfile.js'],
                tasks: ['newer:jshint:all']
            },
            js: {
                files: ['*.js'],
                tasks: ['newer:jshint:all']
            }
        }

    });

    grunt.registerTask('default', 'Watch JS files for errors.', function () {
        var tasks = ['jshint:all', 'watch'];
        grunt.option('force', true);
        grunt.task.run(tasks);
    });

};

