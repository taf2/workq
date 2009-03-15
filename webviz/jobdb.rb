require 'mysql'
require 'job'

class JobDB
  def initialize(config)
    @dbh = Mysql.real_connect(config["host"], config["username"], config["password"], config["database"])
  end
  def finalize
    @dbh.close if @dbh
  end

  def find_all
    res = @dbh.query("select * from jobs")
    _collect(res)
  ensure
    res.free
  end

  def escape(str)
    @dbh.escape_string(str)
  end

  def find(conditions)
    res = @dbh.query("select * from jobs where #{conditions}")
    _collect(res).first
  ensure
    res.free
  end
protected
  def _collect(res)
    records = []
    while row = res.fetch_hash do
      records << Job.new(row)
    end
    records
  end
end
