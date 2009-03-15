require 'socket'
require 'yaml'
require 'rubygems'
require 'mysql'
require 'json'

begin
  # load local server configuration
  config = YAML.load_file(File.join(File.dirname(__FILE__),'..','config','jobs.yml'))

  dbh = Mysql.real_connect(config['host'], config['username'], config['password'], config['database'])

#  res = dbh.query("select * from jobs where id=37")
#  while row = res.fetch_hash do
#    printf "%s, %s\n", row['name'], JSON.parse(row['data']).inspect
#  end
#  puts "Number of rows returned: #{res.num_rows}"
#  res.free
  dbh.query("delete from jobs")

  socket = UDPSocket.open

  100.times do|i|

    data = {
      'original_filename' => 'tests/fixtures/photo.jpg',
      'modified_filename' => "tests/fixtures/thumb#{i}.jpg",
      'operation' => 'resize',
      'rows' => 64, 'cols' => 64
    }

    sleep 0.05
    dbh.query("INSERT INTO jobs (name, status, attempts, locked_queue_id, data, created_at)
                      VALUES    ('magick', 'pending', 0, '', '#{data.to_json}', '#{Time.now}' ) ".squeeze(' '))
    socket.send('1', 0, '127.0.0.1', 4488)
  end
  socket.close
ensure
  dbh.close
end
