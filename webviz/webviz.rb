require 'init'
require 'jobdb'

class Webviz < Merb::Controller
  layout 'default'
  def index
    jobdb = JobDB.new(YAML.load_file("../config/jobs.yml"))
    @jobs = jobdb.find_all
    render
  ensure
    jobdb.finalize if jobdb
  end

  def show
    jobdb = JobDB.new YAML.load_file("../config/jobs.yml")
    @job = jobdb.find("id=#{jobdb.escape(params[:id])}")
    render
  ensure
    jobdb.finalize if jobdb
  end

  def new
    render
  end

  def create
  end

  def edit
    render
  end

  def update
  end

  def delete
    render
  end

  def destroy
  end
end
