# run very flat apps with merb -I <app file>.

# Uncomment for DataMapper ORM
# use_orm :datamapper

# Uncomment for ActiveRecord ORM
# use_orm :activerecord

# Uncomment for Sequel ORM
# use_orm :sequel


#
# ==== Pick what you test with
#

# This defines which test framework the generators will use.
# RSpec is turned on by default.
#
# To use Test::Unit, you need to install the merb_test_unit gem.
# To use RSpec, you don't have to install any additional gems, since
# merb-core provides support for RSpec.
#
# use_test :test_unit
use_test :rspec
require 'mysql'
#
# ==== Choose which template engine to use by default
#

# Merb can generate views for different template engines, choose your favourite as the default.

use_template_engine :erb
# use_template_engine :haml

Merb::Config.use { |c|
  c[:framework]           = { :public => [Merb.root / "public", nil] }
  c[:session_store]       = 'none'
  c[:exception_details]   = true
	c[:log_level]           = :debug # or error, warn, info or fatal
  c[:log_stream]          = STDOUT
  # or use file for loggine:
  # c[:log_file]          = Merb.root / "log" / "merb.log"

	c[:reload_classes]   = true
	c[:reload_templates] = true
}



Merb::Router.prepare do
  match('/').to(:controller => 'webviz', :action =>'index').name(:all_jobs)
  match('/job/new').to(:controller => 'webviz', :action =>'new').name(:new_job)
  match('/job/create').to(:controller => 'webviz', :action =>'create').name(:create_job)
  match('/job/edit').to(:controller => 'webviz', :action =>'edit').name(:edit_job)
  match('/job/update').to(:controller => 'webviz', :action =>'update').name(:update_job)
  match('/job/delete').to(:controller => 'webviz', :action =>'delete').name(:delete_job)
  match('/job/destroy').to(:controller => 'webviz', :action =>'destroy').name(:destroy_job)
  match('/job/:id').to(:controller => 'webviz', :action =>'show').name(:show_job)
end
