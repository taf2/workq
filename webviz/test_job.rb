require 'test/unit'
require 'yaml'
require 'rubygems'
require 'jobdb'

class JobTest < Test::Unit::TestCase
  def setup
    @jobdb = JobDB.new YAML.load_file("../config/jobs.yml")
  end
  def teardown
    @jobdb.finalize
  end

  def test_finder
    jobs = @jobdb.find_all
    assert_equal 7, jobs.size
    for job in jobs do
      assert job.respond_to?(:name)
      assert job.respond_to?(:id)
      assert job.respond_to?(:status)
      assert job.respond_to?(:details)
      assert job.respond_to?(:data)
      assert job.respond_to?(:locked_queue_id)
      assert job.respond_to?(:updated_at)
      assert job.respond_to?(:created_at)
    end
  end
end
