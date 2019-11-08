const path = require('path');
const webpack = require('webpack');
const BabiliPlugin = require("babili-webpack-plugin");
const HtmlWebpackPlugin = require('html-webpack-plugin');
const CompressionPlugin = require('compression-webpack-plugin');

module.exports = {
	mode : 'development',
	resolve : {
		modules : [
				path.resolve(__dirname, "lib/application"),
				path.resolve(__dirname, "lib/binio"),
				path.resolve(__dirname, "lib/tempsensor"),
				path.resolve(__dirname, "files"), "node_modules"
		]
	},
	entry : {
		index : './files/index.js'
	},
	output : {
		path : path.join(__dirname, 'files'),
		filename : '[name].[hash:10].js'
	},
	module : {
		rules : [ {
			test : /\.(js|jsx)$/,
			exclude : /node_modules/, // This may not be needed since we supplied `include`.
			include : path.resolve(__dirname, 'src'),

			/*
			  https://goo.gl/99S6sU
			  Loaders will be applied from right to left.
			  E.x.: loader3(loader2(loader1(data)))
			 */
			use : [
			// https://goo.gl/EXjzoG
			{
				loader : 'babel-loader',
				options : {
					presets : [
					/*
					  To get tree shaking working, we need the `modules: false` below.
					  https://goo.gl/4vZBSr - 2ality blog mentions that the issue is caused
					  by under-the-hood usage of `transform-es2015-modules-commonjs`.
					  https://goo.gl/sBmiwZ - A comment on the above post shows that we
					  can use `modules: false`.
					  https://goo.gl/aAxYAq - `babel-preset-env` documentation.
					 */
					[ '@babel/preset-env', {
						targets : {
							browsers : [ 'last 2 versions' ]
						},
						modules : false
					// Needed for tree shaking to work.
					} ], '@babel/preset-env', // https://goo.gl/aAxYAq
					//'@babel/preset-react' // https://goo.gl/4aEFV3
					],

					// https://goo.gl/N9gaqc - List of Babel plugins.
					plugins : [
					//          '@babel/plugin-proposal-object-rest-spread', // https://goo.gl/LCHWnP
					//          '@babel/plugin-proposal-class-properties' // https://goo.gl/TE6TyG
					]
				}
			} ]
		}

		]
	},
	devtool : 'source-map',
	plugins : [
	new BabiliPlugin(),
	new HtmlWebpackPlugin({
//template : path.resolve('./', './files/index-template.html'),
		template : './files/index-template.html',
//		filename : path.resolve('./', './files/index_new.html'),
		inject : 'head'
	}),
	new CompressionPlugin({
		deleteOriginalAssets : true
	})
	]
}
